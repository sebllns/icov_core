#include "icov_math.h"

#define BOOST_TEST_DYN_LINK 1
#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_MODULE Icov core test
#include <boost/test/unit_test.hpp>
//#include <boost/test/included/unit_test.hpp>

#include "ldpc_codec_layer/BitSymbolProvider.h"

BOOST_AUTO_TEST_SUITE(binary_tests)

BOOST_AUTO_TEST_CASE(bit_operations) {

  for (int i = 0; i < 16; i++) {
    int val = icov::maths::pow2(i);

    BOOST_REQUIRE_MESSAGE(val == std::pow(2, i),
                          std::to_string(val) + " == 2^" + std::to_string(i));

    int length = (int)icov::maths::binary_length(val);
    int expected_length = i + 1;
    BOOST_REQUIRE_MESSAGE(length == expected_length,
                          std::to_string(length) +
                              " == " + std::to_string(expected_length));
  }

  /*    // seven ways to detect and report the same error:
      BOOST_CHECK( add( 2,2 ) == 4 );        // #1 continues on error

      BOOST_REQUIRE( add( 2,2 ) == 4 );      // #2 throws on error

      if( add( 2,2 ) != 4 )
        BOOST_ERROR( "Ouch..." );            // #3 continues on error

      if( add( 2,2 ) != 4 )
        BOOST_FAIL( "Ouch..." );             // #4 throws on error

      if( add( 2,2 ) != 4 ) throw "Ouch..."; // #5 throws on error

      BOOST_CHECK_MESSAGE( add( 2,2 ) == 4,  // #6 continues on error
                           "add(..) result: " << add( 2,2 ) );

      BOOST_CHECK_EQUAL( add( 2,2 ), 4 );	*/  // #7 continues on error
}

BOOST_AUTO_TEST_CASE(bit_symboles) {
  std::vector<icov::SymboleVal> symboles(8192);
  std::iota(symboles.begin(), symboles.end(), 0);

  BOOST_REQUIRE(
      symboles ==
      icov::ldpc_codec_layer::BitSymbolProvider::compute_symboles(
          icov::ldpc_codec_layer::BitSymbolProvider::compute_bits(symboles)));
}
BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE(args_tests)

BOOST_AUTO_TEST_CASE(no_arg) {
  // DOES NOT WORK on WINDOWS
  // BOOST_CHECK_NE(WEXITSTATUS(std::system("./icov_cmd")), 0);
  // BOOST_CHECK_EQUAL(WEXITSTATUS(std::system("./icov_cmd -h")), 0);
  // BOOST_CHECK_EQUAL(WEXITSTATUS(std::system("./icov_cmd --help")), 0);
  // BOOST_CHECK_NE(WEXITSTATUS(std::system("./icov_cmd --coder=\"nocoder\" -n
  // 1024 --datapath=\"aaa\"")), 0);
  // BOOST_CHECK_NE(WEXITSTATUS(std::system("./icov_cmd --coder=\"nocoder\" -n
  // 1024 --data-path=\"aaa\"")), 0);
}

BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE(io)

BOOST_AUTO_TEST_CASE(vector) {
  std::vector<icov::SymboleVal> values(8192);
  std::iota(values.begin(), values.end(), 0);

  icov::io::write_vector("./vector_test", values, '\n');

  std::vector<icov::SymboleVal> values_parsed;
  icov::io::read_vector("./vector_test", values_parsed);
  BOOST_REQUIRE(values == values_parsed);
}

BOOST_AUTO_TEST_SUITE_END();
