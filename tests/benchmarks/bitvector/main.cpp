#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <aff3ct.hpp>
//#include <boost/filesystem.hpp>
#include "common.h"
#include "tools/DebugTools.h"
#include "video_layer/icov_stream.h"
#include <boost/dynamic_bitset.hpp>

using namespace aff3ct;
using namespace icov;

/****************************************************************************************/
// I - AFF3CT CONFIG & STRUCTS
/****************************************************************************************/
struct params {};

void init_params(params &p, int argc, char *argv[]) {
  // if(argc < 6) throw "bad arguments";
}

/****************************************************************************************/
// III - MAIN
/****************************************************************************************/
int main(int argc, char *argv[]) {
  auto start_main = std::chrono::steady_clock::now();

  srand((unsigned)time(NULL));

  /****************************************************************************************/
  // 1 - INIT
  /****************************************************************************************/

  params p;
  init_params(p, argc, argv);

  /****************************************************************************************/
  // TESTS STRUCTURES PERFORMANCES (TODO : move into dedicated program)
  /****************************************************************************************/

  int nv = 10000;

  auto start = std::chrono::steady_clock::now();

  bitstream::BitVector vec(nv);

  for (int k = 0; k < 1000; k++) {
    vec.set(0, 1);
    vec.set(500, 1);
    vec.set(1000, 1);
    for (int i = 1; i < nv; i++) {
      vec.set(i, vec[i] ^ vec[i - 1]);
    }
  }
  auto end = std::chrono::steady_clock::now();

  std::chrono::duration<double> elapsed_seconds = end - start;
  std::cout << "bitset=" << elapsed_seconds.count() / 1000 << std::endl;

  getchar();

  start = std::chrono::steady_clock::now();
  std::vector<bool> vec2(nv);

  for (int k = 0; k < 1000; k++) {
    vec2[0] = 1;
    vec2[500] = 1;
    vec2[1000] = 1;
    for (int i = 1; i < nv; i++) {
      vec2[i] = vec2[i] ^ vec2[i - 1];
    }
  }
  end = std::chrono::steady_clock::now();

  elapsed_seconds = end - start;
  std::cout << "vector=" << elapsed_seconds.count() / 1000 << std::endl;

  getchar();

  start = std::chrono::steady_clock::now();
  icov::FastArray<icov::bit> vec3(nv);

  for (int k = 0; k < 1000; k++) {
    vec3[0] = 1;
    vec3[500] = 1;
    vec3[1000] = 1;
    for (int i = 1; i < nv; i++) {
      vec3[i] ^= vec3[i - 1];
    }
  }
  end = std::chrono::steady_clock::now();

  elapsed_seconds = end - start;
  std::cout << "FastArray=" << elapsed_seconds.count() / 1000 << std::endl;

  getchar();

  start = std::chrono::steady_clock::now();
  boost::dynamic_bitset<> vec4(nv);

  for (int k = 0; k < 1000; k++) {
    vec4[0] = 1;
    vec4[500] = 1;
    vec4[1000] = 1;
    for (int i = 1; i < nv; i++) {
      vec4[i] ^= vec4[i - 1];
    }
  }
  end = std::chrono::steady_clock::now();

  elapsed_seconds = end - start;
  std::cout << "boost::dynamic_bitset=" << elapsed_seconds.count() / 1000
            << std::endl;

  getchar();

  return 0;
}
