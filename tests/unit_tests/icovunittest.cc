// Test module entry point; the other test files only add suites to it
#define BOOST_TEST_DYN_LINK 1
#define BOOST_TEST_MODULE Icov core test
#include <boost/test/unit_test.hpp>

#include "icov_io.h"
#include "icov_math.h"
#include "ldpc_codec_layer/BitSymbolProvider.h"

#include <cmath>
#include <filesystem>
#include <numeric>

BOOST_AUTO_TEST_SUITE(binary_tests)

BOOST_AUTO_TEST_CASE(bit_operations) {
  for (int i = 0; i < 16; i++) {
    BOOST_TEST_CONTEXT("i=" << i) {
      const int val = icov::maths::pow2(i);
      BOOST_CHECK_EQUAL(val, std::pow(2, i));
      BOOST_CHECK_EQUAL((int)icov::maths::binary_length(val), i + 1);
    }
  }
}

BOOST_AUTO_TEST_CASE(bit_symboles) {
  using icov::ldpc_codec_layer::BitSymbolProvider;
  std::vector<icov::SymboleVal> symboles(8192);
  std::iota(symboles.begin(), symboles.end(), 0);

  BOOST_CHECK(symboles == BitSymbolProvider::compute_symboles(
                              BitSymbolProvider::compute_bits(symboles)));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(io)

BOOST_AUTO_TEST_CASE(vector) {
  const auto path =
      (std::filesystem::temp_directory_path() / "icov_vector_test").string();
  std::vector<icov::SymboleVal> values(8192);
  std::iota(values.begin(), values.end(), 0);

  icov::io::write_vector(path, values, '\n');
  std::vector<icov::SymboleVal> values_parsed;
  icov::io::read_vector(path, values_parsed);
  std::filesystem::remove(path);

  BOOST_CHECK(values == values_parsed);
}

BOOST_AUTO_TEST_SUITE_END()
