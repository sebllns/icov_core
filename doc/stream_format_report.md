# ICOV encoder output: current layout and target format

Analyzed output: `build/release/bin/encoded.icov/`, the 1024x512 synthetic gradient, one frame, 512 blocks of 32x32, QP 27, encoder 1.9.0.

Status: this report describes the 1.9.0 layout. Version 2.0.0 implements the single-file target (section 3) with a few differences, listed in `bitstream_report.md` section 12: the container uses ISOBMFF boxes, the statistics file is `name.stats.json`, reference frames are opt-in (`--export-reference`), and debug text goes to `name.debug.txt`.

Objectives:

1. A single file to stream from.
2. A clear separation between the data a client needs to decode (stream) and everything else (index for the server, analysis metrics, debug frames, human-readable info).

## 1. Current layout

`encoded.icov/` is a directory of 8 entries. The decoder (`InputIcov`, `lib/extractor/InputMedia.cpp:23-37`) opens the six binary parts and fails if any is missing.

| File | Size | Share | Content | Written by | Needed by the client |
|---|---|---|---|---|---|
| `header` | 8 276 B | 1.4% | `ContentParameterSet` as a Boost binary archive: version string, dimensions, QP, viewport, frame count, `icu_count`, `encoded_blocks` map | `writeContentParameterSet` | Yes |
| `prediction` | 158 337 B | 27.5% | Per block, per prediction, per channel: prediction mode, q-ary parameters, stored syndrome size and rate index per bitplane (1 byte each) | `writeQaryParams`, `writeSIRate`, `writeSIMode` | Only the selected prediction of each requested block |
| `syndrome` | 132 245 B | 23.0% | Per block: the shared syndrome bits, byte-aligned per bitplane | `writeSyndrome` | A prefix per requested block |
| `bsize` | 4 099 B | 0.7% | Per block: two `uint32` (bytes in `prediction`, bytes in `syndrome`) | `writeBlockSize` | No: server-side index |
| `fsize` | 11 B | 0.0% | Per frame: two `uint32` (same, per frame) | `writeFrameSize` | No: server-side index |
| `metric` | 2 051 B | 0.4% | Per block: encoder MSE (`float`) | `writeMse` | No: analysis only |
| `info.txt` | 825 B | 0.1% | Human-readable summary and rate histograms | `IcovEncoder.cpp:104` | No |
| `source_frames/` | 269 874 B | 46.9% | `x<f>.tif` (reconstruction), `v<f>.tif` (original after resize) | `FrameEncoder.cpp:470-477`, `ExportRecoFrames = true` by default | No: debug and quality reference |
| **Total** | **575 718 B** | | | | |

Stream data proper (`header` + `prediction` + `syndrome`): 298 858 B, 52% of the directory.

## 2. Issues

### 2.1 Structure

| # | Issue | Evidence | Impact |
|---|---|---|---|
| S1 | One stream split over six files | `getIcovPartName()` (`encoded_stream.h:96-113`), each part opened as a separate `ifstream` | Serving a block means seeking in three files. No single file to put behind a CDN or an HTTP range server |
| S2 | Data of one block scattered | A block's parameters are in `prediction`, its syndrome in `syndrome`, its offsets in `bsize`, its MSE in `metric` | At least two disjoint reads per block, plus an index file to compute offsets |
| S3 | Mandatory analysis data | `metric` must exist (`assert_file_exists` on every part), and the extractor reads it for every block (`BlockExtractor::fill_metrics`) | Analysis data cannot be removed from what is served |
| S4 | Debug frames inside the stream directory | `source_frames/` written by default, 47% of the directory | Doubles the size on disk, mixes debug with stream |
| S5 | Index computed at load time | `readBlockPos()` accumulates `bsize` into absolute offsets at open; `fsize` is read by seeking backwards (`InputMedia.cpp:230-265`) | No explicit table of absolute offsets; random access to frame N needs the previous frames' sizes |
| S6 | Unexplained terminators | `endIcov()` appends three `0xFF` bytes to every part except the header; nothing reads them (`BinaryOutputIcov.cpp`, end) | Breaks "file size = data size" assumptions, undocumented |

### 2.2 Header

| # | Issue | Evidence | Impact |
|---|---|---|---|
| H1 | Boost binary archive | `boost::archive::binary_oarchive(..., no_header)`: native byte order and type sizes, strings and containers prefixed with native lengths (raw bytes: `0000 0000 0005 0000 0000 0000 0031 2e39 2e30` for `"1.9.0"`). Boost documents binary archives as non-portable; `no_header` also drops the archive version | Works between Linux and macOS (x86_64, ARM64, same Boost). Breaks on Windows, where `Block::id_t` (`unsigned long`) is 4 bytes instead of 8, and Boost encoding changes go undetected. A client in another language (browser, Python) would have to reimplement Boost's internal encoding |
| H2 | `encoded_blocks` map dominates the header | `std::map<Block::id_t, Block::id_t>` with `id_t = unsigned long`: 512 entries x 16 B = 8 KB of the 8.3 KB header. With all blocks encoded it is the identity | 98% of the header is redundant. A flag "all blocks" or a bitmap of `icu_count` bits (64 B here) is enough |
| H3 | Fields missing from the serialization | `ldpc_coder_type`, `ldpc_bp_ite`, code length and `q_values` are in the struct but not in `serialize()` (`encoded_stream.h:196-228`) | The decoder depends on command-line options (`-c`, `-n`, `--ldpc-bp-ite`) to match the encoder; a mismatch fails silently or decodes wrongly |
| H4 | No magic number, no format version | The file starts with the version string length | A truncated or foreign file is not detected; version checks compare the software version string only (warning, `InputMedia.cpp:57-61`) |
| H5 | Unused flags | `has_metadata` is serialized but never read or set; `numcode_byte_count` exists but is not serialized | Dead fields in the format |

### 2.3 Payload

| # | Issue | Evidence | Impact |
|---|---|---|---|
| P1 | Side-information parameters larger than the syndromes | `prediction` 158 KB vs `syndrome` 132 KB: 309 B of parameters per block, one byte per bitplane, per prediction (9) and per channel (3), plus q-ary parameters | Parameters cost more than the coded data. They are fixed-length bytes, highly redundant (see the rate histograms in `info.txt`: most values are 0, 16 or 136) |
| P2 | Parameters of all predictions stored and skipped sequentially | `selectPrediction()` walks `jumpPrediction()` for every preceding prediction (`InputMedia.cpp:159-185`) | Selecting prediction k needs parsing predictions 0..k-1; no direct offset |
| P3 | Debug prints in the extractor | `std::cout << "COMP "`, `" bitplaneCount "`, `"(int) = "` (`InputMedia.cpp:136-194`) | Noise on stdout during decoding (see the logging report) |

### 2.4 Side outputs

| # | Issue | Evidence | Impact |
|---|---|---|---|
| D1 | `info.txt` path and content | Written next to the stream; lists absolute input and output paths and the `info.txt` path itself | Leaks local paths if the directory is published; the histograms are useful but not machine-readable |
| D2 | Reference frame names | `x<f>.tif` = reconstruction, `v<f>.tif` = original; folder named `source_frames` | Unclear, see B9 in the logging report |
| D3 | MSE per block only | `metric` holds the encoder MSE, used by the viewer CSV and PSNR display | Useful for analysis, but belongs with other encoder statistics, not in the stream |

## 3. Target

### 3.1 Outputs of `icov_cli encode -o name`

| Output | Purpose | Default |
|---|---|---|
| `name.icov` | Single stream file: everything a client needs, nothing else | Always |
| `name.stats.json` | Encoder statistics: parameters, timing, sizes per section, rate histograms, per-block MSE and rates | Always (small) |
| `name.ref/` (`original_<f>.png`, `reconstructed_<f>.png`) | Quality references for PSNR and the viewer difference view | `--export-reference` only |

Nothing else is written. `info.txt` is replaced by `name.stats.json` (or printed with `icov_cli info name.icov`), without absolute paths.

### 3.2 `name.icov` layout

Little-endian, fixed-width fields, no Boost serialization. Offsets are absolute from the start of the file.

```
+--------------------------------------------------------------+
| File header (fixed size)                                     |
|   magic "ICOV", format version (u16), flags (u16)            |
|   offset and size of: sequence header, index, payload        |
+--------------------------------------------------------------+
| Sequence header                                              |
|   content type, width, height, chroma format, frame count,   |
|   frame rate, block size, QP                                 |
|   viewport: width, height, fov x/y, padding                  |
|   LDPC: code family, code length, BP iterations,             |
|         q-ary table id                                       |
|   block set: "all" flag or bitmap of icu_count bits          |
+--------------------------------------------------------------+
| Index (server-side, the client never downloads it)           |
|   per frame: offset of its block table                       |
|   per block: offset and size of each prediction's parameters,|
|              offset of the syndrome, stored size per bitplane|
+--------------------------------------------------------------+
| Payload, frame by frame, block by block                      |
|   [block unit]                                               |
|     parameters, prediction 0 .. 8 (directly addressable)     |
|     syndrome (shared, each prediction reads a prefix)        |
+--------------------------------------------------------------+
```

Properties:

- One file, one `open`, random access by offset. A server answers a request (block, prediction, frame) with two byte ranges: the parameters of that prediction and the syndrome prefix. A plain HTTP server with range requests is enough if the client knows the index; otherwise a small server extracts the ranges.
- The index sits before the payload, so a server reads header and index once and keeps them in memory. The encoder writes the payload first to a temporary file and assembles the final file at the end (encoding is offline).
- Each block is contiguous, so serving a viewport touches few disk regions.
- The format is parseable from any language: a Python reader for tests and tools becomes simple.
- Unknown sections can be skipped by offset and size, so optional data (for example temporal parameters for videos) can be added later without breaking older readers.

### 3.3 Where each current item goes

| Current | Target | Notes |
|---|---|---|
| `header` | Sequence header | Portable fields; add LDPC code family, length, BP iterations (H3); replace `encoded_blocks` by a flag or bitmap (H2); add magic and format version (H4) |
| `bsize`, `fsize` | Index | Absolute offsets, per prediction (fixes S5, P2) |
| `prediction` | Payload, block unit, parameters | Fixed order of predictions, directly addressable |
| `syndrome` | Payload, block unit, syndrome | Unchanged content |
| `metric` | `name.stats.json` | Not read by the decoder any more; the viewer reads it only for analysis views (S3) |
| `info.txt` | `name.stats.json` | No absolute paths (D1) |
| `source_frames/` | `name.ref/`, opt-in | Clear names (D2), off by default (S4) |
| `0xFF` terminators | Removed | Section sizes are in the file header (S6) |

## 4. Plan

| Step | Change | Effort |
|---|---|---|
| 1 | Make `metric` optional in `InputIcov` and stop writing `source_frames/` by default; add `--export-reference` | 1 to 2 h |
| 2 | Serialize the missing header fields (LDPC code, length, BP iterations) and use them in the decoder instead of the command line | 2 h |
| 3 | Writer and reader for the single-file container, behind the current `OutputIcovBase` / `IInputIcov` interfaces, so the encoder and extractor logic does not change. Keep the directory format readable for one release | 1 to 2 days |
| 4 | `name.stats.json` replacing `info.txt` and holding the per-block MSE | 3 h |
| 5 | Python reader (`scripts/icov_info.py`) for inspection and tests; integration test: encode, check section sizes, decode from the single file | 3 h |
| 6 | Compact parameter coding (variable-length or entropy-coded rate indices) to reduce P1, measured against the current 309 B per block | to be estimated |

Steps 1 and 2 are independent of the container and remove the most fragile behavior (analysis data required, decoder parameters taken from the command line). Step 3 delivers the single file.
