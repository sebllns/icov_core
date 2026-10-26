#define BOOST_TEST_DYN_LINK 1
#include <boost/test/unit_test.hpp>

#include "ldpc_codec_layer/factory/CoderFactory.h"

#include <algorithm>
#include <cmath>
#include <random>

using namespace icov;
using namespace icov::ldpc_codec_layer;

namespace {

p_InteractiveCoder make_coder(int n) {
  auto params = createLdpcParams("imt164");
  params->database = ICOV_TEST_DATA_DIR;
  params->codeLength = n;
  params->beliefPropagationIterations = 100;
  return LdpcCoderFactory::create_coder(*params);
}

BitVec random_bits(int n, std::mt19937 &gen) {
  std::bernoulli_distribution d(0.5);
  BitVec v(n);
  for (auto &b : v)
    b = d(gen);
  return v;
}

// Side information LLRs: each bit goes through its own binary symmetric
// channel, with a crossover probability drawn in [p_min, p_max]. Bit
// reliabilities therefore vary, as with real side information.
LLRVec channel(const BitVec &src, double p_min, double p_max,
               std::mt19937 &gen) {
  std::uniform_real_distribution<double> crossover(p_min, p_max);
  std::uniform_real_distribution<double> u(0, 1);
  LLRVec llr(src.size());
  for (size_t k = 0; k < src.size(); k++) {
    const double p = crossover(gen);
    const bool si = src[k] ^ (u(gen) < p);
    llr[k] = static_cast<LLRVal>((si ? -1 : 1) * std::log((1 - p) / p));
  }
  return llr;
}

struct Encoded {
  LLRVec llr;
  NumcodeIdx idx;
  Syndrome synd;
};

Encoded encode(InteractiveCoder<AffMatrix> &coder, const BitVec &src,
               const LLRVec &llr) {
  Encoded e{llr, 0, Syndrome(0)};
  coder.syndromeMgr()->init_accumulation(src); // as in encode_bitplane
  coder.llrProvider()->setLLR(llr);
  BOOST_REQUIRE_EQUAL(coder.encode(e.idx, e.synd, src), 0);
  return e;
}

// Decode from a fresh LLR state, as the decoder does
BitVec decode(InteractiveCoder<AffMatrix> &coder, const LLRVec &llr,
              NumcodeIdx idx, const Syndrome &synd) {
  coder.llrProvider()->setLLR(llr);
  BitVec out;
  coder.decode(out, idx, synd);
  return out;
}

} // namespace

BOOST_AUTO_TEST_SUITE(ldpca_coder)

BOOST_AUTO_TEST_CASE(conditional_entropy_matches_bsc_cost) {
  std::mt19937 gen(1);
  auto coder = make_coder(1024);
  const BitVec src = random_bits(1024, gen);

  // Constant crossover p: the estimate is the mean coding cost of the source
  // under the channel model, -log2(1 - p) or -log2(p) per bit
  const double p = 0.05;
  const LLRVec llr = channel(src, p, p, gen);
  int flips = 0;
  for (size_t k = 0; k < src.size(); k++)
    flips += ((llr[k] < 0) != (src[k] == 1));
  const double q = double(flips) / src.size();
  const double expected = (1 - q) * -std::log2(1 - p) + q * -std::log2(p);
  BOOST_REQUIRE_GT(flips, 0);

  coder->llrProvider()->setLLR(llr);
  BOOST_CHECK_CLOSE(coder->conditional_entropy(src), expected, 0.1);
}

BOOST_AUTO_TEST_CASE(encode_decode_roundtrip) {
  const std::vector<std::pair<double, double>> channels = {
      {0.005, 0.02}, {0.02, 0.08}, {0.05, 0.15}, {0.1, 0.2}};
  for (int n : {256, 1024}) {
    std::mt19937 gen(n);
    auto coder = make_coder(n);
    for (const auto &c : channels) {
      BOOST_TEST_CONTEXT("n=" << n << " p=[" << c.first << "," << c.second
                              << "]") {
        const BitVec src = random_bits(n, gen);
        const Encoded e = encode(*coder, src, channel(src, c.first, c.second, gen));

        BOOST_CHECK_LE(coder->get_r(e.idx), 1.0f);
        BOOST_CHECK(decode(*coder, e.llr, e.idx, e.synd) == src);
      }
    }
  }
}

// A block stores one syndrome per bitplane, the longest across its
// predictions, and every prediction reads a prefix of it. The syndrome bits
// must therefore not depend on the side information, doped bits included.
BOOST_AUTO_TEST_CASE(shared_syndrome_prefix) {
  const std::vector<std::pair<double, double>> channels = {
      {0.01, 0.05}, {0.03, 0.1}, {0.05, 0.15}, {0.08, 0.15}, {0.1, 0.2}};
  for (int n : {256, 1024}) {
    std::mt19937 gen(10 + n);
    auto coder = make_coder(n);
    const BitVec src = random_bits(n, gen);

    std::vector<Encoded> encoded;
    for (const auto &c : channels) {
      BOOST_TEST_CONTEXT("n=" << n << " p=[" << c.first << "," << c.second
                              << "]") {
        encoded.push_back(
            encode(*coder, src, channel(src, c.first, c.second, gen)));
      }
    }

    // Only meaningful if several predictions use doped rates (above 1/2)
    const auto doped = std::count_if(
        encoded.begin(), encoded.end(),
        [&](const Encoded &e) { return coder->get_r(e.idx) > 0.5f; });
    BOOST_REQUIRE_GE(doped, 2);

    const auto &longest =
        *std::max_element(encoded.begin(), encoded.end(),
                          [](const Encoded &a, const Encoded &b) {
                            return a.synd.size() < b.synd.size();
                          });
    for (const auto &e : encoded) {
      BOOST_TEST_CONTEXT("n=" << n << " rate=" << coder->get_r(e.idx)) {
        Syndrome prefix = longest.synd;
        prefix.resize(e.synd.size());
        BOOST_CHECK(prefix == e.synd);
        BOOST_CHECK(decode(*coder, e.llr, e.idx, prefix) == src);
      }
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()
