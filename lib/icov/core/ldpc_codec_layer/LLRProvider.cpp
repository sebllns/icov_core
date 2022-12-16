#include "LLRProvider.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>

#include "helpers.h"

icov::LLRProvider::LLRProvider(int n) : m_llr(n, 0), m_floor_type(0) {
  reset();
}

icov::LLRProvider::LLRProvider(const std::vector<LLRVal> &llr)
    : m_llr(llr), m_floor_type(0) {
  reset();
}

void icov::LLRProvider::initLLR_s(const BitSymbolProvider &bsp,
                                  const std::vector<SIVal> &side_info) {
  reset();

  for (size_t i = 0; i < side_info.size(); i++) {
    LLRVal p = static_cast<LLRVal>(bsp.pqks(side_info[i]));
    p = std::clamp(p, epsilon(), (LLRVal)(1.0 - epsilon()));
    m_llr[i] = std::log(p / (1.0 - p));
  }
}

void icov::LLRProvider::initLLR_b(int b, const BitSymbolProvider &bsp,
                                  const std::vector<SIVal> &side_info,
                                  const std::vector<int> &sign,
                                  const std::vector<int> &abs_predec) {
  reset();

  int l = bsp.x_binary_length() - 1;
  LLRVal p = 0.0, p0 = 0.0, p1 = 0.0;
  for (size_t i = 0; i < side_info.size(); i++) {
    bsp.pqkb(sign[i] > 0, abs_predec[i], l, b, side_info[i], p0, p1, p);
    p = std::clamp(p, epsilon(), (LLRVal)(1.0 - epsilon()));
    m_llr[i] = std::log(p / (1.0 - p));
  }
}

bool icov::LLRProvider::no_coding(const BitVec &src) const {
  bool nocoding = true;
  int i = -1;

  while (nocoding && ++i < (int)this->m_llr.size()) {
    // b = 1 => llr < 0, b = 0 => llr >= 0
    nocoding = ((this->m_llr[i] < 0) == src[i]);
  }

  return nocoding;
}

void icov::LLRProvider::initLLR(float pcrossover, const BitVec &src_bits) {
  icov::helpers::assert::equal(m_llr.size(), src_bits.size(),
                               "Length "
                               "of arrays are not equal");

  reset();

  for (int k = 0; k < (int)src_bits.size(); k++) {
    int sideinfo_BSC =
        ((int)(src_bits[k] +
               (((((double)rand() / (double)RAND_MAX) < pcrossover)) ? 1.0
                                                                     : 0.0))) %
        2;
    double pCond =
        (1 - pcrossover) * (1 - sideinfo_BSC) + pcrossover * sideinfo_BSC;
    m_llr[k] = std::log(pCond / (1 - pCond));
  }
}

void icov::LLRProvider::initLLR(const std::vector<SIVal> &side_info,
                                const BitVec &src_bits) {
  icov::helpers::assert::equal(m_llr.size(), src_bits.size(),
                               "Length "
                               "of arrays are not equal");

  reset();

  float pcrossover = 0;

  for (int k = 0; k < (int)src_bits.size(); k++) {
    if (side_info[k] != src_bits[k]) {
      pcrossover++;
    }
  }

  pcrossover /= src_bits.size();

  for (int k = 0; k < (int)src_bits.size(); k++) {
    double pCond =
        (1 - pcrossover) * (1 - side_info[k]) + pcrossover * side_info[k];
    m_llr[k] = std::log(pCond / (1 - pCond));
  }
}

void icov::LLRProvider::initLLRUniform(float pcrossover,
                                       const BitVec &src_bits) {
  icov::helpers::assert::equal(m_llr.size(), src_bits.size(),
                               "Length "
                               "of arrays are not equal");

  reset();

  float one_count = pcrossover * m_llr.size();
  one_count = m_floor_type ? std::ceil(one_count) : std::floor(one_count);

  for (int k = 0; k < (int)src_bits.size(); k++) {
    m_llr[k] = k < one_count ? 1 : 0;
  }

  std::random_device rd;
  std::mt19937 g(rd());

  std::shuffle(m_llr.begin(), m_llr.end(), g);

  for (int k = 0; k < (int)src_bits.size(); k++) {
    m_llr[k] = (int)(m_llr[k] + src_bits[k]) % 2;
    double pCond = (1 - pcrossover) * (1 - m_llr[k]) + pcrossover * m_llr[k];
    m_llr[k] = log(pCond / (1 - pCond));
  }

  m_floor_type = !m_floor_type;
}

std::vector<icov::BitVal> icov::LLRProvider::initSrcRand(int n) {
  BitVec src(n, 0);
  std::generate(src.begin(), src.end(), []() {
    return (BitVal)(((((double)rand() / (double)RAND_MAX) < 0.5)) ? 1.0 : 0.0);
  });
  return src;
}

void icov::LLRProvider::init_doping() {
  if (!m_doping_ready) {
    ICOV_DEBUG << "Compute doping";
    m_llr_doping = m_llr;
    // icov::sort_indexes_abs(m_llr, m_doping_order);
    m_doping_ready = true;
    //    icov::helpers::io::write_vector(std::cout, m_llr, ' ');
    //    std::cout << std::endl << std::endl;
    //    icov::helpers::io::write_vector(std::cout, m_doping_order, ' ');
  }
}

void icov::LLRProvider::apply_doping(int index, BitVal x) {
#ifdef ICOV_ASSERTION_ENABLED
  icov::helpers::assert::check(m_doping_ready,
                               "Cannot use doping if not initialized");
  icov::helpers::assert::check(index >= 0 && index < (int)m_doping_order.size(),
                               "Index is out of bounds");
#endif

  m_llr_doping[m_doping_order[index]] =
      x > 0 ? std::numeric_limits<LLRVal>::lowest()
            : std::numeric_limits<LLRVal>::max();
}

int icov::LLRProvider::doping_order(int index) const {
  return m_doping_order[index];
}

uint32_t icov::LLRProvider::index_min() const {
  uint32_t idx = 0;

  for (int i = 0; i < (int)m_llr.size(); i++) {
    if (std::fabs(m_llr[i]) < std::fabs(m_llr[idx])) {
      idx = i;
    }
  }
  return idx;
}

icov::LLRVal icov::LLRProvider::limit_max() {
  return std::numeric_limits<LLRVal>::max();
}

icov::LLRVal icov::LLRProvider::epsilon() {
  return std::numeric_limits<LLRVal>::epsilon();
}

void icov::LLRProvider::reset() {
  std::fill(m_llr.begin(), m_llr.end(), 0);
  std::fill(m_llr_doping.begin(), m_llr_doping.end(), 0);
  m_doping_order.resize(m_llr.size());
  std::iota(m_doping_order.begin(), m_doping_order.end(), 0);
  m_doping_ready = false;
}

bool icov::LLRProvider::doping_ready() const { return m_doping_ready; }
