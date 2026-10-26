# ICOV bitstream: analysis and proposed schema

This report covers the bitstream itself: what is stored, what is transmitted, and how to encode it. The file layout of the 1.9.0 encoder output (six files, debug frames, `info.txt`) is analyzed in `stream_format_report.md`; this report builds on it.

Status: sections 1 to 11 describe version 1.9.0 and the proposal. Steps 1 to 6 are implemented in version 2.0.0 (tag `v2.0.0`); section 12 describes what was built and the measured results.

Goals, in order:

1. Transmit the fewest bytes possible for each viewport request.
2. Separate strictly what a decoder needs from index, metadata and debug data.
3. Stay close to the structure of standard codecs and containers, so the format can later be carried by, or proposed to, a standard.

## Measurement setup

All figures come from one encode and one decoded navigation path, run from `build/release/bin` with the release build (`ICOV_USE_OMP=ON`, core version 1.9.0).

| Step | Command or source | Output |
|---|---|---|
| Test image | `python scripts/gen_synthetic_erp.py erp.ppm` | 1024x512 ERP, smooth color gradient |
| Encode | `./icov_cli -i erp.ppm -o encoded.icov/ -d data/imt_164_v2/` (defaults: QP 27, 32x32 blocks, IMT164 LDPCA codes, n = 1024 luma and 256 chroma, 100 BP iterations) | `encoded.icov/`: 512 blocks, one frame |
| Storage sizes (2.1) | `ls -l encoded.icov` | Size of each part |
| Bitplane and rate statistics (2.1, 4.6) | Histograms in `encoded.icov/info.txt` (stored rates, rates per prediction) | 13 671 stored bitplanes (26.7 per block, 28% at rate 0); 96 085 per-prediction rate values; order-0 entropy 2.53 bits, computed from the histogram |
| Viewpoint path | `positions.csv`: 20 viewpoints, 12 on the equator every 30 degrees, 4 up at longitude 0, 4 down at longitude 90 | |
| Decode (2.2) | `./icov_viewer -i encoded.icov -d data/imt_164_v2/ --decode-positions-input positions.csv --decode-csv-output results/viewer3` | `results/viewer3_blocks.csv`: one line per decoded block (prediction, transmitted syndrome bits, all-intra bits) |
| Transmitted bytes (2.2, 7) | Sum of the syndrome bits of blocks not served from the cache; parameter bytes counted from the extracted-stream syntax pushed by `ChannelExtractor` | 452 blocks sent, 888 from the cache; 31 B of parameters per block |

Caveats:

- The syndrome bits in the CSV are exact bit counts; the byte padding per bitplane is not included.
- The CSV includes the cache metrics fix of core commit `e44a3e7`; exports made before it attributed stale values to cached blocks.

These measurements cover a single, favorable content (see "Dependence on content and resolution" in section 2): they give orders of magnitude and the structure of the costs, not general values.

## 1. What ICOV streams

ICOV is closer to distributed source coding than to a classic codec. A block is encoded once, for every way it may be decoded:

- Each block is split into channels (Y, Cb, Cr) and bitplanes of quantized coefficients. Each bitplane is one LDPCA codeword.
- For each block, the encoder considers up to 9 predictions: 8 spatial neighbors and the previous frame (inter). For each prediction it stores the side-information parameters (generation mode, q-ary distribution parameters) and the syndrome rate needed per bitplane.
- The syndrome is shared: it is stored once, at the largest rate needed by any prediction, and a decoder using prediction p reads only a prefix of it.
- Access blocks (a sparse grid) also carry the raw bitplanes, so decoding can start without any neighbor.

At decoding time the client knows which neighbors it already holds, so it chooses the prediction. When both an intra neighbor and the previous frame are available, the cheaper of the two is selected from the stored rates (`BlockExtractor::_processBlock` does this today); in the target design the server makes that choice and signals it with one flag. The server then sends, for each needed block, only the parameters of that prediction and the corresponding syndrome prefix. Blocks already in the client cache are not sent.

This gives two distinct bitstreams. The code already separates them at block level (section 2.2), but only in memory, inside one process:

| Bitstream | Holder | Content |
|---|---|---|
| Storage stream | Server | Everything, for all predictions: parameters of every prediction, full stored syndromes, raw access data, index |
| Transmitted stream | Client | Per request, per block: one prediction's parameters and a syndrome prefix |

The first is written once and read by one program, the server: compactness matters little, random access matters a lot. The second is what goes over the network: every bit counts, and random access is not needed because the client receives exactly what it asked for, in the order it asked.

## 2. Current bitstream

### 2.1 Storage

| Part | Size | Per block | Content |
|---|---|---|---|
| `header` | 8 276 B | | Boost binary archive of `ContentParameterSet`; 8 KB of it is an identity map of block ids |
| `prediction` | 158 337 B | 309 B | Per channel: bitplane count, stored rate per bitplane; per prediction: mode byte, header byte, z_min and z_max bytes, one rate byte per bitplane |
| `syndrome` | 132 245 B | 258 B | Per bitplane: stored syndrome, padded to a byte boundary; raw bitplanes for access blocks |
| `bsize`, `fsize` | 4 110 B | 8 B | Byte sizes per block and per frame, accumulated into offsets at load time |
| `metric` | 2 051 B | 4 B | Encoder MSE, analysis only |

Other observations:

- 26.7 bitplanes per block on average (3 channels); 28% of them have a stored rate of 0.
- About 7 predictions are available per block (inter is absent for an image, border blocks have fewer neighbors): 187.7 rate bytes per block.
- Access blocks follow a grid (blocks 1, 117, 193, 200, 208, 216, 307, 426 on the test path), yet `info.txt` reports "Total number of encoded access blocks 0".
- Every field is a byte or a native integer, independent of its range. A prediction mode or a rate index fits in a few bits.
- The prediction order is fixed (`PredictionID` in `encoded_stream.h`), yet each prediction repeats a mode byte, and selecting prediction k means parsing predictions 0 to k-1.
- The q-ary parameters and the stored sizes are byte values with no entropy coding.

### Dependence on content and resolution

| Part | Depends on resolution (block count) | Depends on content and QP |
|---|---|---|
| `header` | Yes: about 70 B fixed, plus 16 B per block for the block map (8 KB of the 8.3 KB here) | No |
| `prediction` | Yes, linearly | Yes, through the bitplane count per block and the share of coded predictions. Block size also changes it |
| `syndrome` | Yes, linearly | Strongly: it follows how well neighbors predict a block. A smooth gradient is the favorable case |
| `bsize`, `metric` | Yes: fixed 8 B and 4 B per block and frame | No |
| `fsize` | Per frame only, 8 B | No |

The per-block figures (309 B of parameters, 258 B of syndrome) and the shares in section 2.2 are therefore valid only for this synthetic 1024x512 gradient at QP 27. On natural content, syndromes are expected to grow more than parameters, which lowers the parameter share of the traffic. A lower QP adds bitplanes and increases both. For a video, inter prediction adds one parameter set per block.

What does not depend on content is the overhead of the current format: one byte per field, padding per bitplane, and the block map in the header growing with resolution.

Measurements to complete: natural images, several QPs and resolutions, and a video.

### 2.2 Transmission

The transmitted stream exists, in memory. The decoding pipeline has a server role and a client role, connected by a byte buffer:

- Server role: `BlockExtractor` and `ChannelExtractor` (`lib/extractor/`) read the storage parts through `InputIcov`, select the prediction (and inter versus intra), and push the data of that prediction only into an `IcovExtractedBitstream` (`lib/icov/core/video_layer/extracted_stream.h`).
- Client role: `BlockDecoder` and `ChannelDecoder` (`lib/decoder/`) pop from that buffer and never access the storage files.

The pushed syntax, per block and channel (`ChannelExtractor::extract_block_syndrome`, `_pushPredParams`, `_extractBitplaneSyndrome`):

| Field | Size |
|---|---|
| bitplane count | 1 byte |
| SI mode | 1 byte |
| prediction header (coding flag, q index) | 1 byte |
| z_min, z_max (if coded) | 2 bytes |
| rate index per bitplane (if coded) | 1 byte each |
| syndrome prefix per bitplane | rate bits, padded to a byte |

This is already the transmitted stream of section 4.5, with these limits:

- In-process only: an 8 KB ring buffer (`BITSTREAM_SIZE 8192`) filled and emptied block by block; never serialized, written to a file or sent over a network.
- Implicit syntax: defined only by the order of `push` and `pop` calls, with no unit header, no version and no specification.
- One byte per field and byte padding per bitplane, as in storage.
- Not self-sufficient: the sequence parameters do not go through it; the decoder side gets them from the storage header (`SinglePassDecoder` opens `InputIcov`). Requests do not exist as messages either: extractor and decoder share the same request object and block ordering.
- The extractor also reads analysis data (`metric`) on the server side and passes it through a global singleton (`VideoMetrics`), outside the stream.

To measure what this stream carries, the path of 20 viewpoints was decoded and the bytes counted per category (`results/viewer3_blocks.csv`):

| Category | Blocks | Bytes | Share | Note |
|---|---|---|---|---|
| Syndrome prefixes of predicted blocks | 445 | 38 362 | 60% | 86 B per block, exact bit count, before byte padding |
| Parameters of the chosen prediction | 452 | 14 012 | 22% | 31 B per block (mode, header, q-ary, one byte per bitplane) |
| Raw access blocks | 7 | 11 968 | 19% | 1.7 KB each, about 20 predicted blocks |
| **Total** | 452 sent, 888 cached | **64 342** | | 66% of the requested blocks were already cached |

Three conclusions:

- Parameters are 22% of the traffic, coded as raw bytes. This is the easiest reduction.
- Raw access blocks are 19% of the traffic for 7 blocks. Coding them better is a codec change, but the format must allow it.
- The syndromes themselves are the core of the method; the format should only avoid adding padding to them.

## 3. Design principles

1. **Two layers, as in standard codecs.** An elementary stream with its own syntax (what the decoder parses), and a container (how it is stored, indexed and delivered). The elementary stream never contains offsets, file names or statistics.
2. **NAL-unit structure.** Data is split into typed, length-delimited units with a small header, as in H.264, HEVC and VVC. A decoder ignores unit types it does not know; metadata and debug go into separate, droppable unit types.
3. **Bit-level syntax with standard descriptors.** Fields use `u(n)` (fixed bits), `ue(v)` and `se(v)` (Exp-Golomb), `f(n)` (fixed pattern), as in the VVC specification. No byte per field, no native types, no Boost.
4. **Parameter sets sent once.** Everything constant over the sequence (dimensions, block size, QP, LDPC code family and length, BP iterations, projection, viewport model, coding tables) goes in a parameter set sent at session start. Nothing is repeated per block.
5. **Implicit framing in the transmitted stream.** The client asked for a list of (block, prediction) in a given order. The response follows that order, and every length is derivable from the parsed rates, so no block id, prediction id or length field is transmitted.
6. **Context-free coding of block data.** The server combines blocks and predictions differently for every client, so a block payload cannot depend on an adaptive state carried over from other blocks. Coding uses static codes or tables from the parameter set, reset at every block.
7. **Storage optimized for extraction, transmission optimized for size.** The storage keeps per-bitplane byte alignment and an index so that the server can slice quickly; the server repacks the slices at bit level when it builds a response.

## 4. Proposed schema

### 4.1 Overview

```
Elementary stream (NAL units)
  IPS   ICOV parameter set ............ once per sequence
  FH    frame header .................. once per frame (video)
  SBU   storage block unit ............ storage only: all predictions of one block
  TBU   transmitted block unit ........ transmission only: one prediction of one block
  SEI   supplemental information ...... optional, never needed to decode
        (block hash, encoder info, statistics)

Storage container (server)
  ISOBMFF boxes: ftyp, moov (sample entry with IPS, OMAF projection),
  block index box, mdat (SBUs grouped by frame)

Transmission (network)
  session start: IPS
  per request:   request (frame, list of block/prediction) -> response (TBUs in request order)
```

### 4.2 NAL unit header

Two bytes, aligned on the VVC header so that existing tools (parsers, analyzers, encapsulation code) apply with minimal changes:

| Field | Descriptor | Meaning |
|---|---|---|
| `forbidden_zero_bit` | f(1) | 0 |
| `nuh_reserved_zero_bit` | u(1) | 0, reserved |
| `nuh_layer_id` | u(6) | 0, reserved for scalability |
| `nal_unit_type` | u(5) | IPS, FH, SBU, TBU, prefix SEI, suffix SEI; other values reserved |
| `nuh_temporal_id_plus1` | u(3) | 1 for images; temporal layer for videos |

In storage (ISOBMFF) units are length-prefixed (4 bytes), without start codes. In transmission the response is a sequence of TBUs whose lengths are implicit (principle 5); a 2-byte header per response, not per block, is enough. If an Annex B style byte stream is ever needed, start codes and emulation prevention bytes apply as in VVC.

### 4.3 ICOV parameter set (IPS)

Sent once (about 30 to 60 bytes). Replaces the Boost header.

```
icov_parameter_set_rbsp() {
  ips_format_version                      u(8)
  profile_idc                             u(7)   reserved for profiles and levels
  content_type                            u(2)   image, video
  chroma_format_idc                       u(2)
  bit_depth_minus8                        u(4)
  pic_width_in_luma_samples               ue(v)
  pic_height_in_luma_samples              ue(v)
  log2_block_size_minus2                  ue(v)
  init_qp                                 u(6)
  if (content_type == video) {
    frame_count_minus1                    ue(v)
    frame_rate_num                        ue(v)
    frame_rate_den_minus1                 ue(v)
  }
  projection_type                         u(3)   0: ERP (OMAF compatible)
  viewport_hfov_centideg                  u(16)
  viewport_vfov_centideg                  u(16)
  visibility_margin_x100                  u(8)
  ldpc_code_family_idc                    u(4)   0: IMT164
  log2_ldpc_luma_length                   ue(v)
  log2_ldpc_chroma_length                 ue(v)
  bp_max_iterations_minus1                ue(v)
  all_blocks_coded_flag                   u(1)
  if (!all_blocks_coded_flag)
    for (i = 0; i < num_blocks; i++)
      block_coded_flag[i]                 u(1)
  access_block_spacing_minus1             ue(v)  or an explicit map
  rate_code_table_present_flag            u(1)
  if (rate_code_table_present_flag)
    rate_code_table()                            static code for rate indices (4.6)
  ips_extension_flag                      u(1)
  if (ips_extension_flag)
    ips_extension_data()
  rbsp_trailing_bits()
}
```

This fixes the fields missing today (LDPC code family, lengths, BP iterations), replaces the 8 KB block map by one flag, and makes the header parseable from any language.

### 4.4 Storage block unit (SBU)

One per block and frame. Laid out for fast slicing, not for size:

```
storage_block_unit_rbsp() {
  block_idx                               ue(v)
  for (c = 0; c < num_channels; c++) {
    num_bitplanes[c]                      ue(v)
    for (b = 0; b < num_bitplanes[c]; b++)
      stored_rate_idx[c][b]               ue(v)
  }
  for (p = 0; p < num_pred_types; p++) {     fixed order: inter, 4 diagonals, 4 sides
    pred_available_flag[p]                u(1)
    if (pred_available_flag[p])
      pred_params(p)                             same syntax as in the TBU (4.5)
  }
  byte_alignment()
  pred_params_offset[...]                         optional, or kept in the container index
  for (c ...) for (b ...) {
    stored_syndrome[c][b]                 u(v)   stored_rate bits
    byte_alignment()                             keeps each bitplane sliceable
  }
  if (is_access_block)
    raw_block_data()                             see 4.7
}
```

The server never interprets the syndrome bits. It parses `pred_params(p)` of the requested prediction, reads the rate indices, and copies the matching syndrome prefixes.

### 4.5 Transmitted block unit (TBU)

What the client receives for one requested (block, prediction). No id, no length, no padding between bitplanes:

```
transmitted_block_unit() {
  if (inter_and_intra_available)
    inter_selected_flag                   u(1)
  for (c = 0; c < num_channels; c++) {
    num_bitplanes[c]                      ue(v)
    pred_params(c)
    for (b = 0; b < num_bitplanes[c]; b++)
      syndrome_prefix[c][b]               u(v)   length derived from rate_idx[c][b]
  }
}

pred_params(c) {
  si_mode                                 u(n)   n from the number of SI modes
  si_coded_flag                           u(1)
  if (si_coded_flag) {
    qary_q_idx                            u(n)
    qary_z_min                            se(v)
    qary_z_max_minus_z_min                ue(v)
  }
  num_leading_zero_rate_bitplanes         ue(v)
  for (b = num_leading_zero_rate_bitplanes; b < num_bitplanes[c]; b++)
    rate_idx_delta[c][b]                  coded with the static table, or se(v)
}
```

A response is the concatenation of the TBUs in request order, followed by one `byte_alignment()`. The client knows the order and parses lengths from the rates, so framing costs nothing.

Access blocks requested without prediction use the same unit with `raw_block_data()` instead of syndromes.

### 4.6 Coding of the rate indices

The rate indices are most of the parameter bytes (26.7 per block and prediction). Measured on the test file, the order-0 entropy of the SI rate values is 2.53 bits, against 8 bits stored today. Options, from simplest to best:

| Method | Expected size per prediction | Notes |
|---|---|---|
| Today: one byte per bitplane | 26.7 B | |
| `ue(v)` on the index | about 10 B | No table, standard descriptor |
| Leading zero bitplanes as a count, then `se(v)` delta to the previous bitplane | about 6 to 8 B | Rates vary smoothly across bitplanes; 28% of stored bitplanes have rate 0 |
| Static table in the IPS (Huffman or tANS over delta values), built by the encoder from the sequence | about 5 to 8 B (entropy bound 8.4 B order-0 on raw values, lower on deltas) | Sent once per sequence, context-free per block |

Recommendation: start with leading-zero count plus `se(v)` deltas (simple, standard descriptors), then add the IPS table if measurements justify it. Adaptive arithmetic coding (CABAC) is excluded for block data by principle 6, unless its contexts are reset at every block, which removes most of its gain on such short sequences.

### 4.7 Access blocks

Today the raw bitplanes are sent: 11 776 to 15 616 bits per block, 19% of the session traffic for 7 blocks. The format reserves an `access_coding_idc` in `raw_block_data()`:

| Value | Coding |
|---|---|
| 0 | Raw bitplanes (current behavior) |
| 1 | Bitplanes with all-zero bitplanes signaled by count |
| 2 | Entropy-coded quantized coefficients, using the VVC residual coding of the same transform (the encoder already uses VTM) |

Value 2 makes an access block a regular intra-coded VVC block. It also opens a compatibility path: access blocks become decodable by any VVC decoder.

### 4.8 Supplemental information (SEI)

All non-decoding data moves to SEI units, which a decoder may drop and a server never transmits by default:

| SEI | Content | Use |
|---|---|---|
| Decoded block hash | CRC or MD5 of each reconstructed block | Conformance and regression tests, like the VVC decoded picture hash SEI |
| Encoder information | Version, options, encode time | Replaces part of `info.txt` |
| Block statistics | Encoder MSE, rate per prediction | Replaces `metric`; analysis tools and the viewer only |

The reference frames (`source_frames/`) stay outside the bitstream entirely.

## 5. Storage container

ISOBMFF (ISO/IEC 14496-12) box syntax is a size, a four-character code and a payload. Adopting it from the start costs no more than a custom container and keeps every later step standard:

```
ftyp   major brand 'icov'
moov
  trak
    mdia / minf / stbl
      stsd  sample entry 'icv1'
              icvC  configuration box: IPS
              OMAF projection boxes (ERP), so 360 players know the geometry
      stsz, stco    one sample per frame
    icbi          block index: per frame, per block, offset and size of its SBU
meta    optional: encoder information, statistics (or separate files)
mdat    SBUs, frame by frame, block by block
```

- For a still image, HEIF (ISO/IEC 23008-12) applies the same boxes with items instead of a track.
- The block index box is the only custom box: per frame, the absolute offset of each SBU. Optionally, offsets of each prediction inside the SBU, so the server reads only the requested parameters.
- Statistics and encoder information live in `meta`, or in a side file, never in `mdat`.

## 6. Transmission

| Step | Content | Size |
|---|---|---|
| Session start | IPS | 30 to 60 B, once |
| Request | Frame number, then the list of (block index, prediction) in decoding order, `ue(v)` coded, block indices as deltas | A few bytes per block |
| Response | 2-byte header, TBUs in request order, final byte alignment | Section 7 |

Two deployment options, both compatible with the same storage file:

- An extraction server parses the request, reads the SBUs through the index and builds the response. Smallest traffic.
- A plain HTTP server with range requests (RFC 9110), if the client downloads the index and the parameters once. More traffic (whole parameter blocks, byte-aligned syndromes), no server logic. Useful for demonstrations.

Viewport-dependent delivery in MPEG-DASH (ISO/IEC 23009-1) and OMAF (ISO/IEC 23090-2) works with tiles chosen by the client; ICOV requests are finer (block and prediction), so a DASH mapping would group blocks into tile-like segments. This is a later step, not a constraint on the elementary stream.

## 7. Expected gains

Per predicted block sent, from the measurements of section 2.2:

| Item | Today | Proposed | Source of the gain |
|---|---|---|---|
| Parameters | 31 B | about 8 to 12 B | Bit fields, implicit prediction order, coded rate indices (4.6) |
| Syndrome | 86 B + byte padding per bitplane | 86 B, one final padding | Bit-level packing in the TBU |
| Framing | none defined | 0 B | Implicit lengths and order |
| **Total per predicted block** | **about 120 B** | **about 95 to 100 B** | **-17% to -21%** |

On the 20-viewpoint session (64.3 KB):

| Item | Today | Proposed |
|---|---|---|
| Parameters (452 blocks) | 14.0 KB | 3.6 to 5.4 KB |
| Syndromes (445 blocks) | 38.4 KB | 38.4 KB |
| Access blocks (7) | 12.0 KB | 12.0 KB with `access_coding_idc = 0`; much less with value 2, to be measured |
| **Total** | **64.3 KB** | **54 to 56 KB (-13% to -16%) before access block coding** |

The parameter estimates come from the order-0 entropy of the measured rate values and from the field ranges; they must be confirmed by an implementation on several contents (natural images, videos with inter prediction).

## 8. Separation of data

| Data | Today | Proposed |
|---|---|---|
| Sequence parameters | `header` (Boost) | IPS, in the sample entry and sent at session start |
| Block parameters and syndromes | `prediction`, `syndrome` | SBU in storage, TBU in transmission |
| Offsets | `bsize`, `fsize` | Container index box, server only |
| Encoder MSE | `metric`, required by the decoder | Block statistics SEI or side file, optional |
| Encoder summary | `info.txt` with absolute paths | Encoder information SEI or `meta`, no paths |
| Reference frames | `source_frames/`, by default | Side directory, opt-in, never in the container |
| Conformance check | none | Decoded block hash SEI |

## 9. Standards mapping

| ICOV element | Standard counterpart | Reference |
|---|---|---|
| NAL units, header, RBSP, trailing bits | VVC NAL unit syntax | ITU-T H.266, ISO/IEC 23090-3 |
| `u(n)`, `ue(v)`, `se(v)` descriptors | Same descriptors | ITU-T H.266, section 7 |
| IPS | Sequence and picture parameter sets | ITU-T H.266 |
| SEI units, decoded block hash | VSEI messages, decoded picture hash | ITU-T H.274, ISO/IEC 23002-7 |
| Storage container, sample entry, configuration box | ISOBMFF, NAL-structured video carriage | ISO/IEC 14496-12, 14496-15 |
| Still image storage | HEIF | ISO/IEC 23008-12 |
| ERP projection, viewport | OMAF projection and viewport boxes | ISO/IEC 23090-2 |
| Viewport-dependent delivery | DASH, spatial relationship description | ISO/IEC 23009-1 |
| Access blocks coded with VVC residual coding | VVC intra blocks | ITU-T H.266 |
| Decoding a block from one of several references | Switching pictures (SP/SI slices) in H.264 | ITU-T H.264, Annex on SP/SI |

ICOV cannot be a VVC profile: its core (LDPCA syndromes decoded with side information) has no counterpart in VVC. What the mapping gives is a format that standard tooling understands at every level except the block payload: NAL parsers, ISOBMFF libraries, OMAF players for the geometry, DASH for delivery. That is the usual path for a new coding tool proposed to MPEG or JVET.

## 10. Migration plan

| Step | Change | Effort |
|---|---|---|
| 1 | Bit writer and reader with `u(n)`, `ue(v)`, `se(v)` (or reuse the VTM `OutputBitstream` / `InputBitstream` classes already in the tree) | 0.5 day |
| 2 | IPS replacing the Boost header, with the missing LDPC fields; decoder takes code, lengths and iterations from it | 1 day |
| 3 | SBU writer and reader behind `OutputIcovBase` / `IInputIcov`; ISOBMFF container with the block index box; `metric` and `info.txt` moved out | 2 to 3 days |
| 4 | Formalize the existing extracted stream as TBUs: bit-level writer in `ChannelExtractor`, reader in `ChannelDecoder`, IPS sent through the same path, request as a message. The extractor and decoder can then run in two processes; the 8 KB ring buffer becomes a per-response buffer | 1 to 2 days |
| 5 | Rate index coding: leading zero count and `se(v)` deltas; measure on several contents; then decide on the IPS table | 1 day, plus measurements |
| 6 | Decoded block hash SEI and a conformance test in CI: encode, extract a path, decode, compare hashes | 1 day |
| 7 | Access blocks with VVC residual coding (`access_coding_idc = 2`) | to be estimated, codec work |
| 8 | OMAF boxes and DASH mapping | later |

Steps 1 to 4 deliver a clean, single-file, standard-shaped format and the first transmitted stream. Steps 5 and 7 bring the byte reductions. Each step keeps the integration test (encode, decode, PSNR, determinism) green.

## 11. Decisions to take

- Storage container: ISOBMFF from the start (recommended), or a minimal custom box format converted later.
- Rate index coding: `se(v)` deltas only, or a per-sequence table in the IPS.
- Transmission protocol: extraction server (smallest traffic) or plain HTTP range requests (no server logic), or both.
- Access block coding: keep raw for now, or plan the VVC residual coding.
- Compatibility of the old directory format: read-only for one release, or dropped.

## 12. Implemented in 2.0.0 (steps 1 to 6)

Released as `v2.0.0` (merged from `feature/bitstream`, one commit per step). All measurements on the same 1024x512 gradient and 20-viewpoint path as above; decoded blocks, predictions, rates and PSNR are identical to 1.9.0 at every step.

### 12.1 What was built

| Step | Implementation | Differences with the proposal |
|---|---|---|
| 1 | `video_layer/bit_io.h`: `BitWriter` / `BitReader`, `u(n)`, `ue(v)`, `se(v)`, unaligned bit copies, byte alignment, trailing bits | VTM `BitStream` not used: no Exp-Golomb helpers, and the extractor library does not link VTM |
| 2 | `video_layer/ips.{h,cpp}`: ICOV parameter set replacing the Boost header. Stores the LDPC code family and BP iterations, which the decoder now uses | Floats (fov, padding, frame rate) stored bit-exact as `u(32)` so encoder and decoder geometry match. Code lengths are not stored: they follow from block size and chroma format. Coded blocks as id deltas minus one (1 bit per block when all are coded) |
| 3 | Single `name.icov` file: `ftyp`, `moov` (`icvC` parameter set, `icbi` block index), `mdat` (one storage block unit per block). Side files: `name.stats.json` (statistics, MSE, no absolute paths), `name.ref/` (only with `--export-reference`), `name.debug.txt` (debug builds) | No `trak` / `stsd` hierarchy and no OMAF boxes yet: `icvC` sits directly under `moov`. The SBU has no `block_idx` field: the index gives its position |
| 4 | `IcovExtractedBitstream` is now a per-block bit buffer carrying the transmitted block unit; the decoder parses the parameter set from its bytes; "transmitted bytes" column in the CSV export | Extractor and decoder still run in one process; requests are not serialized messages yet |
| 5 | `video_layer/block_syntax.h`: one field syntax for SBU and TBU. Rate indices as `ue(q - 1)`, raw source mapped to the largest code | Delta coding across bitplanes rejected after measurement (see 12.3) |
| 6 | CRC-32 of each reconstructed block in an optional `ihsh` box; `icov_cli decode --verify-hash`; used by the integration test. Both programs now return a non-zero exit code on failure | |

### 12.2 Measurements

| | 1.9.0 | 2.0.0 |
|---|---|---|
| Stream files | 6 files, 305 019 B (header 8 276, prediction 158 337, syndrome 132 245, bsize 4 099, fsize 11, metric 2 051) | 1 file, 236 210 B |
| Parameter set / header | 8 276 B (Boost archive) | 123 B (`icvC`) |
| Block parameters and syndromes | 290 582 B | 225 779 B (`mdat`, -22%) |
| Index | 4 110 B (`bsize`, `fsize`) | 8 212 B (`icbi`, absolute 64-bit offsets) |
| Conformance hashes | none | 2 068 B (`ihsh`, optional) |
| Side data next to the stream | `metric`, `info.txt`, `source_frames/` (270 KB) by default | `stats.json` (20 KB); reference frames only on request |
| Transmitted on the path (452 blocks) | about 64.3 KB (estimate of section 2.2) | 60.6 KB measured |
| Per predicted block | about 117 B | 109.3 B (86.2 B of syndrome, 23.1 B of parameters and padding) |

### 12.3 Rate index coding: measured options

On the 96 085 rate indices of the test file:

| Coding | Bits per index |
|---|---|
| 1 byte (1.9.0) | 8 |
| `ue(q)` | 4.28 |
| `ue(q - 1)`, raw source last (implemented) | 3.42 |
| Order-0 entropy | 2.59 |
| `ue(rank)`, one static table per channel and bitplane in the IPS | 2.17 |
| Entropy given channel and bitplane | 1.62 |

Rates are not monotonic across bitplanes (most frequent values 1, 2 and 4, with jumps), so the proposed leading-zero count and `se(v)` deltas would not help. The per-channel, per-bitplane table is the next gain: about 3 B per predicted block, for a table sent once per session.

### 12.4 Remaining overhead and next steps

Per predicted block, the 23.1 B beyond the syndrome are: about 11.4 B of rate indices, 6 B of `z_min` / `z_max` (`u(8)` each, three channels), 3 B of mode and coding header, 2.6 B of bitplane counts, and the final byte alignment. In order of expected gain:

1. Access blocks: still raw, 12 KB of the 60.6 KB on the path (step 7, VVC residual coding).
2. Rate table in the IPS (12.3).
3. `z_min` / `z_max`: code `z_max - z_min` and predict `z_min` from the channel.
4. Requests as messages and extractor and decoder in two processes, to measure real network traffic.
5. `trak` / `stsd` sample entry and OMAF projection boxes (step 8).

## 13. LDPC matrices (2.1.0)

Decoding needs the LDPC matrices used by the encoder. Up to 2.0.0 they were read from a data directory given with `-d` (default `data/`, which does not hold them directly), and the parameter set only recorded the coder type `imt164`, shared by two different matrix sets (`imt_164` and `imt_164_v2`).

Since 2.1.0:

- The default set `imt_164_v2` and the q-ary table are compiled into the library (`cmake/EmbedData.cmake`, `lib/icov/core/common/icov_data.h`): no data directory is needed.
- The parameter set (IPS version 2) records the set name and a CRC-32 of its content, computed over the relative paths and the integers of each file, so independent of text formatting. IPS version 1 files (2.0.0) still decode, with the built-in set.
- A set that is not built in (encoded with `-d <dir>`, different content) is stored in the file, in an optional `ildp` box in `moov`: integers packed at the bit width of each file, 17.6 KB for a set like `imt_164_v2` (46 KB of text). `--external-matrices` disables this; the decoder then needs the same `-d`.
- The decoder looks for a set with the right CRC in the file, then in the built-in set, then in `-d`, and otherwise stops with the set name and CRC.

A path is never stored: it only exists on the machine that encoded the file.

For streaming, the `ildp` box is not part of the block units. A server would send it once per session, only to clients that do not know the set CRC (not implemented: requests are not messages yet). With the built-in set, nothing is sent.
