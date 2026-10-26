#include "video_layer/bit_io.h"

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <vector>

using icov::bitstream::BitReader;
using icov::bitstream::BitWriter;

BOOST_AUTO_TEST_SUITE(bitstream)

BOOST_AUTO_TEST_CASE(fixed_length_roundtrip) {
  BitWriter w;
  w.u(1, 1);
  w.u(3, 5);
  w.u(16, 0xBEEF);
  w.u(32, 0xDEADBEEF);
  w.u(64, 0x0123456789ABCDEFull);
  w.flag(false);
  BOOST_TEST(w.bit_count() == 1u + 3 + 16 + 32 + 64 + 1);

  BitReader r(w.data());
  BOOST_TEST(r.u(1) == 1u);
  BOOST_TEST(r.u(3) == 5u);
  BOOST_TEST(r.u(16) == 0xBEEFu);
  BOOST_TEST(r.u(32) == 0xDEADBEEFu);
  BOOST_TEST(r.u(64) == 0x0123456789ABCDEFull);
  BOOST_TEST(!r.flag());
}

BOOST_AUTO_TEST_CASE(exp_golomb_roundtrip) {
  std::vector<uint64_t> uvals = {0, 1, 2, 3, 6, 7, 254, 255, 1000, 1u << 20};
  std::vector<int64_t> svals = {0, 1, -1, 2, -2, 127, -128, 100000, -100000};

  BitWriter w;
  for (auto v : uvals)
    w.ue(v);
  for (auto v : svals)
    w.se(v);

  BitReader r(w.data());
  for (auto v : uvals)
    BOOST_TEST(r.ue() == v);
  for (auto v : svals)
    BOOST_TEST(r.se() == v);
}

BOOST_AUTO_TEST_CASE(exp_golomb_codes) {
  // ue(v) codes from the H.266 specification: 0 -> 1, 1 -> 010, 4 -> 00101
  BitWriter w;
  w.ue(0);
  w.ue(1);
  w.ue(4);
  BOOST_TEST(w.bit_count() == 9u);
  BOOST_TEST(w.data()[0] == 0xA2); // 1 010 0010 1...
  BOOST_TEST(w.data()[1] == 0x80);
}

BOOST_AUTO_TEST_CASE(unaligned_bits_and_alignment) {
  const uint8_t src[] = {0xF0, 0x0F, 0xA0};
  BitWriter w;
  w.u(3, 0b101);
  w.bits(src, 19); // 0xF0 0x0F then 101
  w.byte_alignment();
  BOOST_TEST(w.aligned());
  BOOST_TEST(w.bit_count() == 24u);
  w.trailing_bits();
  BOOST_TEST(w.bit_count() == 32u);
  BOOST_TEST(w.data()[3] == 0x80);

  BitReader r(w.data());
  BOOST_TEST(r.u(3) == 0b101u);
  uint8_t out[3];
  r.bits(out, 19);
  BOOST_TEST(out[0] == 0xF0);
  BOOST_TEST(out[1] == 0x0F);
  BOOST_TEST(out[2] == 0xA0);
  r.byte_alignment();
  BOOST_TEST(r.position() == 24u);
}

BOOST_AUTO_TEST_CASE(aligned_bits_copy) {
  const uint8_t src[] = {1, 2, 3, 4};
  BitWriter w;
  w.bits(src, 32);
  BOOST_TEST(w.data() == std::vector<uint8_t>(src, src + 4));
}

BOOST_AUTO_TEST_CASE(read_past_end_throws) {
  const uint8_t one = 0xFF;
  BitReader r(&one, 1);
  r.skip(8);
  BOOST_CHECK_THROW(r.u(1), std::exception);
}

BOOST_AUTO_TEST_SUITE_END()

#include "video_layer/ips.h"

BOOST_AUTO_TEST_SUITE(ips)

BOOST_AUTO_TEST_CASE(parameter_set_roundtrip) {
  icov::bitstream::ContentParameterSet cps;
  cps.icov_version = "2.0.0";
  cps.content_type = 1;
  cps.content_chroma_format = 1;
  cps.content_width = 1024;
  cps.content_height = 512;
  cps.qp = 27;
  cps.content_framecount = 3;
  cps.content_framerate = 29.97f;
  cps.viewport_width = 409;
  cps.viewport_height = 230;
  cps.viewport_fov_x = 1.3962634f;
  cps.viewport_fov_y = 0.8819f;
  cps.viewport_padding = 1.2f;
  cps.block_fixed_size = 32;
  cps.ldpc_coder_type = "imt164";
  cps.ldpc_bp_ite = 20;
  cps.ldpc_matrix_set = "imt_164";
  cps.ldpc_matrix_crc = 0x12345678;
  cps.icu_count = 512;
  for (unsigned id = 1; id <= 512; ++id)
    cps.encoded_blocks[id] = id - 1;

  const auto bytes = icov::bitstream::write_ips(cps);
  BOOST_TEST(bytes.size() < 140u); // the block list costs one bit per block
  BOOST_TEST((icov::bitstream::read_ips(bytes) == cps));
}

BOOST_AUTO_TEST_CASE(unknown_ldpc_family_rejected) {
  icov::bitstream::ContentParameterSet cps;
  cps.ldpc_coder_type = "unknown";
  BOOST_CHECK_THROW(icov::bitstream::write_ips(cps), std::exception);
}

BOOST_AUTO_TEST_SUITE_END()

#include "video_layer/block_syntax.h"

BOOST_AUTO_TEST_SUITE(block_syntax)

BOOST_AUTO_TEST_CASE(rate_roundtrip_and_cost) {
  namespace bs = icov::bitstream::block_syntax;
  BitWriter w;
  for (unsigned q = 0; q <= 255; ++q)
    bs::write_rate(w, q);
  BitReader r(w.data());
  for (unsigned q = 0; q <= 255; ++q)
    BOOST_TEST(bs::read_rate(r) == q);

  // the most frequent index (1, no coding) costs a single bit
  BitWriter one;
  bs::write_rate(one, 1);
  BOOST_TEST(one.bit_count() == 1u);
}

BOOST_AUTO_TEST_SUITE_END()

#include "icov_data.h"
#include "video_layer/matrix_set.h"

BOOST_AUTO_TEST_SUITE(matrix_set)

BOOST_AUTO_TEST_CASE(builtin_set_matches_directory) {
  namespace data = icov::data;
  const auto builtin = data::load_set(data::DEFAULT_MATRIX_SET);
  const auto dir = data::load_set(ICOV_TEST_DATA_DIR);
  BOOST_TEST(builtin.size() == dir.size());
  BOOST_TEST(data::set_crc32(builtin) == data::set_crc32(dir));
  BOOST_TEST(data::set_name(ICOV_TEST_DATA_DIR) == "imt_164_v2");
  BOOST_TEST(data::exists(std::string(data::DEFAULT_MATRIX_SET) + "/1024/H.pcm"));
}

BOOST_AUTO_TEST_CASE(stored_set_roundtrip) {
  namespace data = icov::data;
  const auto files = data::load_set(data::DEFAULT_MATRIX_SET);
  const auto payload = icov::bitstream::write_matrix_set("imt_164_v2", files);
  BOOST_TEST(payload.size() < 20000u); // packed integers, not text

  std::string name;
  uint32_t crc = 0;
  const auto back = icov::bitstream::read_matrix_set(payload, name, crc);
  BOOST_TEST(name == "imt_164_v2");
  BOOST_TEST(crc == data::set_crc32(files));
  BOOST_TEST(data::set_crc32(back) == crc);

  // a stored set is readable like the built-in one
  data::register_set("stream:test", back);
  auto in = data::open("stream:test/256/H.pcm");
  int rows = 0, cols = 0;
  *in >> rows >> cols;
  BOOST_TEST(rows == 256);
  BOOST_TEST(cols == 128);
}

BOOST_AUTO_TEST_SUITE_END()
