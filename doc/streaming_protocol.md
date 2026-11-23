# ICOV streaming protocol

Status: specification, protocol version 1, not implemented. Applies to the 2.x stream format (`bitstream_report.md` sections 4 and 12). Function and file references describe core `v2.1.0`.

## 1. Scope and roles

A 360 image or video is encoded once into a `.icov` file stored on a server (on-demand only; live encoding is out of scope). A client navigates and, for each frame and viewpoint, receives the transmitted block units (TBU) of the blocks it has not decoded yet.

Each side keeps the information it owns:

| Side | Owns | Decides |
|---|---|---|
| Client | Decoded blocks and the frame each was last decoded in, viewport, navigation history | Access block, decoding order, intra side information of each block, prefetch margin, what to cancel |
| Server | Stored rates and syndromes (`.icov` file, block index) | INTER or INTRA when both are possible, extraction of the TBU |

The server keeps no state between requests. A request carries everything needed to extract its blocks.

This split follows the current code:

- `RequestManager::processViewport` and `processBlock` (`lib/icov/core/omnidirectional_layer/RequestManager.cpp`) pick the access block, the order (`IBlockOrdering::getNextBlock`) and the prediction (`IBlockPredictionManager::getPrediction`) from the viewport and the ordering state only. They never read decoded data, so a whole viewport can be planned before any data arrives.
- `BlockExtractor::_processBlock` (`lib/extractor/BlockExtractor.cpp`) needs, per block: the frame, the block, its intra prediction, and whether it was decoded in the previous frame (`check_inter`). It compares the stored INTER and INTRA rates and keeps the smaller. It also passes the raw part of access blocks (`Block::isAccessBlock()`), which the server derives from the parameter set geometry.
- The INTER choice is signalled inside the TBU (`si_mode` equal to `NUM_LUMA_MODE`, read by `ChannelDecoder::_set_pred_mode` in `lib/decoder/ChannelDecoder.cpp`), so the client learns it from the data.

### Models not chosen

- Stateful server (the client sends only viewpoint and frame, the server runs the ordering and tracks each client): per-client memory, an exact mirror of the client state that breaks on any lost or cancelled response and needs a resync message, and prefetch decided far from the user.
- Byte ranges from a static HTTP server: the INTER/INTRA choice needs the rates of every prediction, so the client would need them. For 8K ERP with 32x32 blocks (28 800 blocks, 10 predictions, 2 to 3 B each), that is 0.6 to 0.9 MB of index per video frame, or a second round trip per frame to fetch the part of the index for the visible blocks. The stored syndromes are also padded to the longest rate, so a range would carry unused bits unless the storage layout changes (format 3.0).

Both remain possible later: publishing a rate table in the session setup moves toward client-side choices, and nothing in the messages prevents a server from caching per session.

### Request granularity

One request per frame and viewport, not one per block:

- Blocks are not independent: a predicted block can only be decoded after the neighbor that is its side information. Fetching blocks separately gives no reordering freedom.
- A predicted TBU averages 103 B on the test path (section 11). A per-block request costs a round trip and tens of bytes of request headers; at 8K a viewport covers 2 000 to 3 000 blocks per frame.
- One request lists the blocks in decoding order at about 1.2 B per block, and the response streams the units in that order, so the decoder starts on the access block while the rest arrives.

Inside a request, blocks are split into priority groups (for example the visible field of view, then the prefetch margin), so the client can cancel a margin when the viewport moves.

## 2. Framing

Every message is:

| Field | Coding | Meaning |
|---|---|---|
| `type` | `u8` | Message type, section 2.1 |
| `length` | LEB128 unsigned, 1 to 4 bytes | Payload length in bytes, at most 2^24 - 1 |
| `payload` | `length` bytes | Bit-level fields (`u(n)`, `ue(v)`, `se(v)` as in `video_layer/bit_io.h`), then `byte_alignment()`, then raw bytes where stated |

The descriptors and their bit order are those of the stream format (`bitstream_report.md` section 4). Multi-byte fixed fields are big-endian.

ISOBMFF box headers (8 bytes) were not used for messages: one block unit per message makes the header a per-block cost, 4 072 B instead of 1 175 B on the test path (6.5% instead of 1.9% of the TBU bytes). Boxes from the file are carried verbatim as payload where needed.

### 2.1 Message types

| Type | Name | Direction | Purpose |
|---|---|---|---|
| `0x01` | `hello` | client to server | Open a session on a content |
| `0x02` | `info` | server to client | Parameter set and optional data of the content |
| `0x03` | `request` | client to server | Blocks of one frame and viewport, in groups |
| `0x04` | `cancel` | client to server | Stop a request from a group on |
| `0x05` | `group` | server to client | Start of the units of one group |
| `0x06` | `block` | server to client | One TBU |
| `0x07` | `end` | server to client | End of a group, with status |
| `0x08` | `error` | server to client | Session-level error, the server then closes the connection |

A receiver ignores a message with an unknown type (it skips `length` bytes), so later versions can add types.

## 3. Session

### 3.1 `hello`

| Field | Coding | Meaning |
|---|---|---|
| `protocol_version` | `u(8)` | 1 |
| `content_id_length` | `ue(v)` | At most 255 |
| `content_id` | bytes after alignment | Catalog name (UTF-8). Never a path: the server maps names to files |
| `want_hashes` | `u(1)` | Send the `ihsh` box (conformance check, `--verify-hash`) |
| `num_known_sets` | `ue(v)` | Matrix sets the client already holds, built in or cached |
| `known_set_crc[i]` | `u(32)` | CRC-32 of each set, as recorded in the parameter set (`bitstream_report.md` section 13) |

`hello` may be sent again on the same connection to switch content, for example to the next GOP file. Requests in progress are cancelled first.

### 3.2 `info`

| Field | Coding | Meaning |
|---|---|---|
| `protocol_version` | `u(8)` | Version the server speaks, 1 |
| `num_boxes` | `ue(v)` | Number of boxes that follow |
| boxes | bytes after alignment | Boxes copied from the file: `icvC` (parameter set, always), `ildp` (matrix set, only when its CRC is not in `known_set_crc`), `ihsh` (only with `want_hashes`) |

The client builds its geometry, ordering and decoder from `icvC`, as `SinglePassDecoder::openIcovFile` does today with `InputIcov::parameterSetBytes()`. With the built-in matrix set and no hashes, `info` is about 130 B.

## 4. Requests

### 4.1 `request`

| Field | Coding | Meaning |
|---|---|---|
| `request_id` | `ue(v)` | Chosen by the client, increasing within a session |
| `frame` | `ue(v)` | Frame number, below the frame count of the parameter set |
| `num_groups` | `ue(v)` | 1 to 64, in decreasing priority |
| per group: `num_blocks` | `ue(v)` | At least 1 |
| per block: `block_id_delta` | `se(v)` | Block id minus the previous block id of the request (0 before the first) |
| per block: `pred` | `u(4)` | 0: none (access block), 1 to 8: RIGHT, LEFT, TOP, BOTTOM, TOP_RIGHT, TOP_LEFT, BOTTOM_RIGHT, BOTTOM_LEFT (`BlockPrediction::ePredType`) |
| per block: `inter_available` | `u(1)` | The client holds this block decoded at `frame - 1` |

Rules:

- Blocks are listed in decoding order across all groups. A block's side information must be decoded earlier in the same request or in an earlier one.
- The first block of the first group is the access block found by `IAccessBlockManager::findAccessBlock`, with `pred = 0`.
- Blocks already decoded for `frame` are not listed (today `previouslyDecoded == r.frameID` in `BlockExtractor::_processBlock`). A viewpoint whose blocks are all decoded sends no request.
- `inter_available` replaces the `previouslyDecoded` frame passed to the extractor: the server sets `check_inter` from it, and only for `frame > 0`.
- The client may send the next request before the current one ends; the server handles requests in order.

### 4.2 Response

For each group of the request, in order:

1. `group`: `request_id` `ue(v)`, `group_index` `ue(v)`, `num_blocks` `ue(v)`.
2. One `block` per listed block, in the request order. Payload: the TBU bytes exactly as `IcovExtractedBitstream` builds them (byte aligned, `bitstream_report.md` section 4.5). The block is implied by its position in the group, so the message has no header beyond the framing. A `block` with an empty payload means the block could not be extracted (section 6).
3. `end`: `request_id` `ue(v)`, `group_index` `ue(v)`, `status` `ue(v)` (0 complete, 1 cancelled, 2 error), `blocks_sent` `ue(v)`.

The chosen prediction (access, intra direction, or INTER) is read from the TBU by the decoder. The client fills its metrics from the received units: transmitted bytes from the payload sizes, syndrome bits from the rate indices it decodes.

### 4.3 `cancel`

| Field | Coding | Meaning |
|---|---|---|
| `request_id` | `ue(v)` | Request to stop |
| `from_group` | `ue(v)` | First group to drop; that group and all later groups of the request are cancelled |

Later groups may use blocks of the cancelled group as side information, so a cancel always covers the rest of the request. The server finishes the block it is writing, then sends `end` with status 1 and the number of blocks sent for the current group, and `end` with `blocks_sent = 0` for the remaining groups. The client treats the blocks not received as not decoded and plans them again in a later request.

## 5. Transport

| Transport | Use | Mapping |
|---|---|---|
| In-process loopback | Default for `icov_cli -m decode` and `icov_viewer` on a local file | Byte queues between the client and server objects. The local decoder runs through the same messages as the remote one |
| TCP (first network transport) | `icov_cli -m serve`, `--decode-server host:port` | One connection per client, messages in sequence. Boost.Asio, synchronous, one thread per connection. Between two blocks the server checks for pending input (`socket::available()`) to apply a `cancel` |
| QUIC (later, step 8 and 10) | Real network | Session messages on a control stream; one stream per group, priority from the group order, cancellation by resetting the streams |

The transport sits behind one interface (send a message, receive a message), so the client and server code do not depend on it.

## 6. Validation and limits

The server checks every request before extracting:

- `frame` below the frame count, `request_id` greater than the previous one.
- Each block id is a coded block of the parameter set (`encoded_blocks`).
- `pred` is a valid direction for the block border (`BlockPrediction::getAvailablePredictions`), and `pred = 0` only for an access block.
- At most as many blocks as coded blocks per frame, no block listed twice, at most 64 groups.
- Message length at most 64 KB for client messages; content id at most 255 B.

A malformed message, an unknown content or an unsupported version ends the session with `error` (`code` `ue(v)`, `message_length` `ue(v)`, UTF-8 text) and the connection is closed. A block that fails extraction gets an empty `block` and the group ends with status 2; the client stops decoding that group, since later blocks may depend on it.

TLS is not part of the TCP transport (trusted networks and loopback). It is mandatory with QUIC.

## 7. Loss, timeouts and reconnection

- TCP delivers in order without loss; a broken connection loses the requests in progress.
- On reconnection the client sends `hello` again and requests what it still needs. There is no server state to restore.
- The client keeps the decoded frame buffer; blocks not received by their display deadline are shown from the previous frame or left empty. The policy belongs to the player (step 9).

## 8. Measurements to report once implemented

- Bytes on the wire per path, split into requests, framing and TBU bytes.
- Round trips per frame (1 expected) and request latency.
- Equality of in-process and TCP decoding: block hashes and CSV exports identical.

## 9. Implementation outline (Phase B)

1. Message library in `lib/icov/core/protocol/`: writers and readers for each message on top of `BitWriter` / `BitReader`, LEB128 framing, unit tests (round trip of every message, rejection of malformed input).
2. Client: split `SinglePassDecoder::setRequest` into planning and decoding. A collector processor on the `RequestManager` loop records (block, prediction, previous frame) and builds the `request`; received `block` units fill the extracted bitstream and replay `BlockDecoder::processBlock` in the same order. `RequestManager` marks a block decoded only once its unit is decoded.
3. Server: `ExtractionServer` wrapping `InputIcov` and `BlockExtractor`, with the omnidirectional picture built from the parameter set to resolve block ids and access blocks. Validation of section 6.
4. Metrics: `VideoMetrics` filled on the client from the received units; the encoder MSE comes from a local `stats.json` when available, as today.
5. Transports: loopback, then TCP; `icov_cli -m serve -i <dir> --port N` (catalog of the `.icov` files of a directory) and `--decode-server host:port` for `icov_cli` and `icov_viewer`.
6. Tests: unit tests in `tests/unit_tests`; the integration test decodes the path in process and through `serve` on localhost, with `--verify-hash`, and compares the CSV exports.

## 10. Open points

- Rate table in `info`, so the client can predict response sizes for rate control (`bitstream_report.md` section 12.3).
- Deadline field in `request`, if the server should skip late groups itself.
- Multi-frame requests for video prefetch; version 1 sends one request per frame.

## 11. Worked example

Test file `erp.icov` (1024x512 gradient, 512 blocks, one frame, built-in matrix set), decoded with `icov_cli -m decode --decode-positions-input positions.csv` on the 20-viewpoint path of `bitstream_report.md` (12 on the equator every 30 degrees, 4 up at longitude 0, 4 down at longitude 90), core `v2.1.0`. Requests and framing sizes are computed from the exported block list with the syntax above.

| Item | Size |
|---|---|
| Session: `hello` and `info` | about 10 B and 130 B, once |
| Requests sent | 12 of 20 viewpoints (8 are fully decoded already) |
| Blocks requested | 509 (6 access blocks, 503 predicted), 3 to 126 per request |
| Request block lists | 624 B (9.75 bits per block), plus about 6 B of header per request |
| TBU bytes | 62 174 B (access blocks 10 258 B, predicted blocks 103.2 B on average) |
| `block` framing | 1 175 B (1 type byte, 1 or 2 length bytes; 157 units of 128 B or more) |
| `group` and `end` messages | about 12 B per request |
| Total | about 64.3 KB, of which 2.2 KB of protocol overhead (3.4%) |
