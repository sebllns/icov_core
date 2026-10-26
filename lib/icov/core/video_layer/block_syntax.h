#pragma once

#include "video_layer/bit_io.h"

#include <cstdint>

namespace icov {
namespace bitstream {

/// Field coding shared by the storage block unit (SBU) and the transmitted
/// block unit (TBU)
namespace block_syntax {

inline void write_bitplane_count(BitWriter &w, unsigned v) { w.ue(v); }
inline unsigned read_bitplane_count(BitReader &r) { return r.ue(); }

/// Intra direction + 1, 0: prediction not available
inline void write_mode(BitWriter &w, unsigned v) { w.u(7, v); }
inline unsigned read_mode(BitReader &r) { return r.u(7); }

/// q-ary index + 1, 0: block not coded with this prediction
inline void write_pred_header(BitWriter &w, unsigned v) { w.ue(v); }
inline unsigned read_pred_header(BitReader &r) { return r.ue(); }

inline void write_z(BitWriter &w, unsigned v) { w.u(8, v); }
inline unsigned read_z(BitReader &r) { return r.u(8); }

/// Rate index (numcode): 1 (no coding) is the most frequent value and 0
/// (raw source) the rarest, so 1 maps to code 0 and 0 to the largest code.
/// Measured on a test image: 3.4 bits per index instead of 8.
constexpr unsigned RATE_SOURCE_CODE = 255;

inline void write_rate(BitWriter &w, unsigned q) {
  w.ue(q == 0 ? RATE_SOURCE_CODE : q - 1);
}

inline unsigned read_rate(BitReader &r) {
  const unsigned v = r.ue();
  return v == RATE_SOURCE_CODE ? 0 : v + 1;
}

} // namespace block_syntax
} // namespace bitstream
} // namespace icov
