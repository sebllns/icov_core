#pragma once

#include "icov_exception.h"

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <vector>

namespace icov {
namespace bitstream {

/// Minimal ISO/IEC 14496-12 box I/O: big-endian fields, size + four-character code
namespace isobmff {

/// ICOV file layout:
///   ftyp            major brand 'icov'
///   moov
///     icvC          FullBox: ICOV parameter set (IPS)
///     icbi          FullBox: block index, per frame and coded block:
///                   u64 SBU offset from file start, u32 parameter bytes,
///                   u32 syndrome bytes
///     ihsh          FullBox, optional: CRC-32 of each reconstructed block
///   mdat            storage block units (SBU), frame by frame
constexpr char BRAND[] = "icov";

inline void put_u8(std::vector<uint8_t> &out, uint8_t v) { out.push_back(v); }

inline void put_u32(std::vector<uint8_t> &out, uint32_t v) {
  for (int s = 24; s >= 0; s -= 8)
    out.push_back(uint8_t(v >> s));
}

inline void put_u64(std::vector<uint8_t> &out, uint64_t v) {
  put_u32(out, uint32_t(v >> 32));
  put_u32(out, uint32_t(v));
}

inline void put_type(std::vector<uint8_t> &out, const char *type) {
  out.insert(out.end(), type, type + 4);
}

/// Serializes a box around an already built payload
inline std::vector<uint8_t> box(const char *type,
                                const std::vector<uint8_t> &payload) {
  std::vector<uint8_t> out;
  put_u32(out, uint32_t(8 + payload.size()));
  put_type(out, type);
  out.insert(out.end(), payload.begin(), payload.end());
  return out;
}

/// Same with the FullBox version (u8) and flags (u24)
inline std::vector<uint8_t> full_box(const char *type, uint8_t version,
                                     const std::vector<uint8_t> &payload) {
  std::vector<uint8_t> body;
  put_u32(body, uint32_t(version) << 24);
  body.insert(body.end(), payload.begin(), payload.end());
  return box(type, body);
}

/// Header of a box whose payload is written separately (e.g. mdat)
inline std::vector<uint8_t> box_header(const char *type,
                                       uint64_t payload_size) {
  std::vector<uint8_t> out;
  if (payload_size + 8 <= UINT32_MAX) {
    put_u32(out, uint32_t(payload_size + 8));
    put_type(out, type);
  } else {
    put_u32(out, 1); // largesize follows
    put_type(out, type);
    put_u64(out, payload_size + 16);
  }
  return out;
}

inline uint32_t get_u32(const uint8_t *p) {
  return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 |
         p[3];
}

inline uint64_t get_u64(const uint8_t *p) {
  return uint64_t(get_u32(p)) << 32 | get_u32(p + 4);
}

/// Location of a box inside a stream or buffer
struct BoxInfo {
  std::string type;
  uint64_t offset = 0;      ///< start of the box
  uint64_t header_size = 0; ///< 8, or 16 with largesize
  uint64_t size = 0;        ///< whole box

  uint64_t payload_offset() const { return offset + header_size; }
  uint64_t payload_size() const { return size - header_size; }
};

/// Reads the box header at the current position; false at end of stream
inline bool read_box(std::istream &in, BoxInfo &b) {
  uint8_t h[8];
  b.offset = uint64_t(in.tellg());
  if (!in.read(reinterpret_cast<char *>(h), 8))
    return false;
  b.type.assign(reinterpret_cast<char *>(h + 4), 4);
  b.size = get_u32(h);
  b.header_size = 8;
  if (b.size == 1) {
    uint8_t l[8];
    if (!in.read(reinterpret_cast<char *>(l), 8))
      ICOV_THROW_RUNTIME_EXCEPTION("Truncated box header");
    b.size = get_u64(l);
    b.header_size = 16;
  }
  if (b.size < b.header_size)
    ICOV_THROW_RUNTIME_EXCEPTION("Invalid size for box " + b.type);
  return true;
}

/// Lists the boxes contained in a buffer (e.g. the payload of moov)
inline std::vector<BoxInfo> list_boxes(const std::vector<uint8_t> &data) {
  std::vector<BoxInfo> boxes;
  uint64_t pos = 0;
  while (pos + 8 <= data.size()) {
    BoxInfo b;
    b.offset = pos;
    b.type.assign(reinterpret_cast<const char *>(&data[pos + 4]), 4);
    b.size = get_u32(&data[pos]);
    b.header_size = 8;
    if (b.size == 1) {
      b.size = get_u64(&data[pos + 8]);
      b.header_size = 16;
    }
    if (b.size < b.header_size || pos + b.size > data.size())
      ICOV_THROW_RUNTIME_EXCEPTION("Invalid size for box " + b.type);
    boxes.push_back(b);
    pos += b.size;
  }
  return boxes;
}

} // namespace isobmff
} // namespace bitstream
} // namespace icov
