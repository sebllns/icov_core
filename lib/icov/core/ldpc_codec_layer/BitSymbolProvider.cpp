#include "BitSymbolProvider.h"

#include "common_types.h"
#include "helpers.h"
#include <algorithm>
#include <limits>
#include <string>
#include <sys/types.h>

namespace icov {
namespace ldpc_codec_layer {

BitSymbolProvider::BitSymbolProvider(int n)
    : m_src_symboles(n, 0),
      m_src_bits(1 /*we need to save only the sign bitplane to decode*/,
                 BitVec(n, 0)) {}

BitSymbolProvider::BitSymbolProvider(const SymboleVec &src_symbols)
    : m_src_symboles(src_symbols),
      m_src_bits(BitSymbolProvider::compute_bits(src_symbols)) {
  m_distribution.setBitplane_count(m_src_bits.size());
}

Bitplanes BitSymbolProvider::compute_bits(const SymboleVec &src_symbols) {
  int x_min = *std::min_element(src_symbols.begin(), src_symbols.end());
  int x_max = *std::max_element(src_symbols.begin(), src_symbols.end());
  int n = src_symbols.size();

  // get length for binary representation of X
  //    lx =  std::floor(std::log2f(std::max(std::abs(x_min), std::abs(x_max))))
  //    + 2;
  int lx = helpers::maths::binary_length(
               std::max(std::abs(x_min), std::abs(x_max))) +
           1;

  // init bit planes of X
  auto src_bits = Bitplanes(lx, BitVec(n, 0));

  for (int i = 0; i < n; i++) {
    int x_reconstruction = 0;
    src_bits[0][i] = (src_symbols[i] < 0);

    for (int j = 1; j < lx; j++) {
      src_bits[lx - j][i] = (std::abs(src_symbols[i]) >> (j - 1)) & 1U;
      x_reconstruction |= src_bits[lx - j][i] << (j - 1);
    }

    // sanity check for X binarization
    x_reconstruction *= (1 - 2 * src_bits[0][i]);
    if (x_reconstruction != src_symbols[i])
      throw std::runtime_error(
          "ERROR: Binarization failed: " + std::to_string(x_reconstruction) +
          " != " + std::to_string(src_symbols[i]));
  }

  return src_bits;
}

SymboleVec BitSymbolProvider::compute_symboles(const Bitplanes &src_bits) {
  int n = src_bits[0].size();
  int lx = src_bits.size();

  auto symboles = BitVec(n, 0);

  for (int i = 0; i < n; i++) {
    BitVal x_reconstruction = 0;
    for (int j = 1; j < lx; j++) {
      x_reconstruction |= src_bits[lx - j][i] << (j - 1);
    }

    symboles[i] = x_reconstruction * (1 - 2 * src_bits[0][i]);
  }

  return symboles;
}

void BitSymbolProvider::inject_bitplane(SymboleVec &output, const BitVec &input,
                                        int shift) {

  ICOV_ASSERT_MSG(output.size() == input.size(),
                  "Vectors must be same size for this operation");

  for (size_t i = 0; i < output.size(); ++i) {
    if (shift < 0) /*apply sign bitplane*/
    {
      if (input[i])
        output[i] *= -1;

#if ICOV_ASSERTION_ENABLED
      ICOV_ASSERT_MSG(output[i] < (1 << bitplane_count()),
                      "Value overflows limits");
#endif
    } else {
      output[i] = (output[i] << shift) + input[i];
    }
  }
}

void BitSymbolProvider::set_si(const SIVec &si) {

  ICOV_ASSERT_MSG(si.size() == m_src_symboles.size(),
                  "SI must be same size as source " +
                      std::to_string(si.size()) +
                      "==" + std::to_string(m_src_symboles.size()));

  m_si_symboles = si;
}

void BitSymbolProvider::set_si_and_compute_pz_params(
    const SIVec &si, const QaryDistribution::quantization_params &quantParams) {
  set_si(si);
  m_distribution.quantizationParams() = quantParams;
  m_distribution.compute_pz_params(m_src_symboles, bitplane_count(), si);
}

void BitSymbolProvider::set_si_and_qaryDistribution(
    const SIVec &si, const QaryDistribution &dist) {
  set_si(si);
  m_distribution = dist;
}

void BitSymbolProvider::reset_src() {
  std::fill(m_src_symboles.begin(), m_src_symboles.end(), 0);
}

// float BitSymbolProvider::pz(int zmin, int zmax) const {
//  float p = 0;

//  for (int z = zmin; z <= zmax; z++) {
//    p += this->pz(z);
//  }
//  return p;
//}

int BitSymbolProvider::bitplane_count() const {
  return distribution().getBitplaneCount();
}

int BitSymbolProvider::code_length() const { return m_src_symboles.size(); }

void BitSymbolProvider::injectBitplane(uint i, BitVec x) {

  if (i == 0) {
    // save sign
    m_src_bits[0] = x;
  } else {
    if (i < m_src_bits.size())
      m_src_bits[i] = x;

    // apply decoded bitplane
    inject_bitplane(m_src_symboles, x, 1);

    // if last apply sign
    if (i == ((uint)m_distribution.getBitplaneCount() - 1)) {
      inject_bitplane(m_src_symboles, m_src_bits[0], -1);
    }
  }
}

void BitSymbolProvider::resetBitplanes(int count) {
  if ((int)m_src_bits.size() != count) {
    m_src_bits = Bitplanes(count, BitVec(code_length(), 0));
  } else {
    for (auto &bp : m_src_bits)
      std::fill(bp.begin(), bp.end(), 0);
  }
}

const QaryDistribution &BitSymbolProvider::distribution() const {
  return m_distribution;
}
QaryDistribution &BitSymbolProvider::distribution() { return m_distribution; }

} // namespace ldpc_codec_layer
} // namespace icov
