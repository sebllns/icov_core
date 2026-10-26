#include "ips.h"

#include "icov_exception.h"

#include <cstring>
#include <string>

namespace icov {
namespace bitstream {

namespace {

// ldpc_code_family_idc values
const char *const LDPC_FAMILIES[] = {"imt164"};

uint32_t float_bits(float f) {
  uint32_t u;
  std::memcpy(&u, &f, sizeof(u));
  return u;
}

float bits_float(uint32_t u) {
  float f;
  std::memcpy(&f, &u, sizeof(f));
  return f;
}

void write_string(BitWriter &w, const std::string &s) {
  w.ue(s.size());
  for (char c : s)
    w.u(8, static_cast<uint8_t>(c));
}

std::string read_string(BitReader &r) {
  std::string s(r.ue(), '\0');
  for (auto &c : s)
    c = static_cast<char>(r.u(8));
  return s;
}

} // namespace

void write_ips(BitWriter &w, const ContentParameterSet &cps) {
  w.u(8, IPS_FORMAT_VERSION);
  write_string(w, cps.icov_version);

  w.u(4, cps.content_type);
  w.u(4, cps.content_chroma_format);
  w.ue(cps.content_width);
  w.ue(cps.content_height);
  w.u(6, cps.qp);
  w.ue(cps.content_framecount);
  // Floats are stored bit-exact so that encoder and decoder geometry match
  w.u(32, float_bits(cps.content_framerate));
  w.u(32, float_bits(cps.content_duration));

  w.flag(cps.is_block_fixed_size);
  w.ue(cps.block_fixed_size);

  w.ue(cps.viewport_width);
  w.ue(cps.viewport_height);
  w.u(32, float_bits(cps.viewport_fov_x));
  w.u(32, float_bits(cps.viewport_fov_y));
  w.u(32, float_bits(cps.viewport_padding));

  unsigned family = 0;
  const unsigned family_count = sizeof(LDPC_FAMILIES) / sizeof(*LDPC_FAMILIES);
  while (family < family_count && cps.ldpc_coder_type != LDPC_FAMILIES[family])
    ++family;
  if (family == family_count)
    ICOV_THROW_INVALID_ARGUMENT("Unknown LDPC coder type: " +
                                cps.ldpc_coder_type);
  w.u(4, family);
  w.ue(cps.ldpc_bp_ite - 1);
  write_string(w, cps.ldpc_matrix_set);
  w.u(32, cps.ldpc_matrix_crc);

  // Coded blocks in stream order, as id deltas minus one
  w.ue(cps.icu_count);
  w.ue(cps.encoded_blocks.size());
  omnidirectional_layer::Block::id_t prev = 0;
  size_t index = 0;
  for (const auto &b : cps.encoded_blocks) {
    // the stream index is implicit: blocks are coded in increasing id order
    if (b.second != index++)
      ICOV_THROW_LOGICAL_EXCEPTION("Coded blocks must follow the id order");
    w.ue(b.first - prev - 1); // ids strictly increase, from 1
    prev = b.first;
  }

  w.flag(false); // ips_extension_flag
  w.trailing_bits();
}

std::vector<uint8_t> write_ips(const ContentParameterSet &cps) {
  BitWriter w;
  write_ips(w, cps);
  return w.data();
}

ContentParameterSet read_ips(BitReader &r) {
  ContentParameterSet cps;

  const unsigned version = r.u(8);
  if (version < 1 || version > IPS_FORMAT_VERSION)
    ICOV_THROW_INVALID_ARGUMENT("Unsupported IPS version " +
                                std::to_string(version));
  cps.icov_version = read_string(r);

  cps.content_type = r.u(4);
  cps.content_chroma_format = r.u(4);
  cps.content_width = r.ue();
  cps.content_height = r.ue();
  cps.qp = r.u(6);
  cps.content_framecount = r.ue();
  cps.content_framerate = bits_float(r.u(32));
  cps.content_duration = bits_float(r.u(32));

  cps.is_block_fixed_size = r.flag();
  cps.block_fixed_size = r.ue();

  cps.viewport_width = r.ue();
  cps.viewport_height = r.ue();
  cps.viewport_fov_x = bits_float(r.u(32));
  cps.viewport_fov_y = bits_float(r.u(32));
  cps.viewport_padding = bits_float(r.u(32));

  const unsigned family = r.u(4);
  if (family >= sizeof(LDPC_FAMILIES) / sizeof(*LDPC_FAMILIES))
    ICOV_THROW_INVALID_ARGUMENT("Unknown LDPC code family " +
                                std::to_string(family));
  cps.ldpc_coder_type = LDPC_FAMILIES[family];
  cps.ldpc_bp_ite = r.ue() + 1;
  if (version >= 2) {
    cps.ldpc_matrix_set = read_string(r);
    cps.ldpc_matrix_crc = r.u(32);
  } // version 1 (2.0.0): imt_164_v2, crc unknown

  cps.icu_count = r.ue();
  const size_t coded = r.ue();
  omnidirectional_layer::Block::id_t id = 0;
  for (size_t i = 0; i < coded; ++i) {
    id += r.ue() + 1;
    cps.encoded_blocks[id] = i;
  }

  if (r.flag())
    ICOV_THROW_INVALID_ARGUMENT("IPS extensions are not supported");
  return cps;
}

ContentParameterSet read_ips(const std::vector<uint8_t> &data) {
  BitReader r(data);
  return read_ips(r);
}

} // namespace bitstream
} // namespace icov
