#pragma once

#include "InteractiveCoder.h"

#include "FastArray.h"
#include "icov_assert.h"
#include "icov_exception.h"
#include "icov_log.h"
#include "icov_types.h"
#include "ldpc_codec_layer/interface/IInteractiveCoder.h"

#include <algorithm>
#include <exception>

namespace icov {
namespace ldpc_codec_layer {

template <class Matrix> size_t InteractiveCoder<Matrix>::get_n() const {
  return m_psyndromeMgr->get_n();
}

template <class Matrix>
int InteractiveCoder<Matrix>::get_m(const NumcodeIdx &numcodeIdx) const {
  return m_psyndromeMgr->get_m(m_matrix_provider->getNumcode(numcodeIdx));
}

template <class Matrix> int InteractiveCoder<Matrix>::get_m_min() const {
  return m_psyndromeMgr->get_m(m_matrix_provider->getNumcode(0));
}

template <class Matrix> int InteractiveCoder<Matrix>::get_m_max() const {
  return m_psyndromeMgr->get_m(
      m_matrix_provider->getNumcode(m_matrix_provider->getNumCodesCount() - 1));
}

template <class Matrix>
Rate InteractiveCoder<Matrix>::get_r(const NumcodeIdx &numcodeIdx) const {
  return get_r(get_m(numcodeIdx), get_n());
}

template <class Matrix>
Rate InteractiveCoder<Matrix>::get_r(int m, int n) const {
  return static_cast<Rate>(m) / static_cast<Rate>(n);
}

template <class Matrix>
bool InteractiveCoder<Matrix>::testConvergence(const BitVec &decoded,
                                               const BitVec &source) const {
  if (decoded.size() != source.size())
    return false;

  for (size_t i = 0; i < decoded.size(); i++) {
    if (decoded[i] != source[i])
      return false;
  }

  return true;
}

template <class Matrix>
int InteractiveCoder<Matrix>::errorCount(const BitVec &decoded,
                                         const BitVec &source) const {
  float numerrors = 0;
  for (size_t i = 0; i < decoded.size(); i++) {
    numerrors += std::abs(decoded[i] - source[i]);
  }
  return (int)numerrors;
}

template <class Matrix>
Rate InteractiveCoder<Matrix>::conditional_entropy(const BitVec &source) const {
  // Ideal Slepian-Wolf rate: mean of -log2 P(x_i | llr_i), with llr < 0 <=> 1
  const auto &llr = m_pllrProvider->llr();
  double bits = 0;
  for (size_t k = 0; k < source.size(); k++)
    bits += std::log1p(std::exp(source[k] ? llr[k] : -llr[k]));
  return static_cast<Rate>(bits / (std::log(2.0) * source.size()));
}

template <class Matrix>
int InteractiveCoder<Matrix>::encode(NumcodeIdx &i, Syndrome &o_stored_syndrome,
                                     const BitVec &source) {
  // Rates are searched from the conditional entropy estimate: exponential
  // steps up to the first converging rate, then bisection down. Each attempt
  // starts from a fresh doping state, as the decoder does, so that a lower
  // rate never benefits from bits doped by a previously tried higher rate.
  // Rates above 1 are skipped: storing the bitplane raw is cheaper.
  constexpr Rate estimate_margin = 0.05f;

  const int n = static_cast<int>(get_n());
  int last = m_matrix_provider->getNumCodesCount() - 1;
  while (last > 0 && get_m(last) > n)
    last--;

  m_pllrProvider->reset_doping();
  const Rate start_rate = conditional_entropy(source) - estimate_margin;
  int start = static_cast<int>(i);
  while (start < last && get_r(start) < start_rate)
    start++;

  BitVec decoded(n);
  int stored_k = -1;
  auto converges = [&](int k) {
    stored_k = k;
    m_pllrProvider->reset_doping();
    o_stored_syndrome.resize(0);
    m_psyndromeMgr->compute_stored(
        o_stored_syndrome, m_matrix_provider->getNumcode(k),
        *m_pllrProvider.get(), source);
    this->decode(decoded, k, o_stored_syndrome);
    return testConvergence(decoded, source);
  };

  // Exponential search: lo fails (or is below start), hi converges
  int lo = start - 1;
  int hi = start;
  int step = 1;
  while (!converges(hi)) {
    if (hi == last) {
      i = last;
      return errorCount(decoded, source);
    }
    lo = hi;
    hi = std::min(hi + step, last);
    step *= 2;
  }

  // Bisection between lo and hi
  while (hi - lo > 1) {
    const int mid = lo + (hi - lo) / 2;
    if (converges(mid))
      hi = mid;
    else
      lo = mid;
  }

  // Restore the syndrome of the selected rate if the last attempt was lower
  if (stored_k != hi)
    converges(hi);

  i = hi;
  return 0;
}

template <class Matrix>
int InteractiveCoder<Matrix>::encode(uint32_t &io_numcode,
                                     Syndrome &o_stored_syndrome,
                                     const BitVec &source, uint32_t numcodeMax,
                                     uint32_t step) {
  BitVec decoded(m_psyndromeMgr->get_n());

  uint32_t next_test_code = io_numcode;
  for (io_numcode = 0; io_numcode < numcodeMax; io_numcode++) {
    m_psyndromeMgr->compute_stored(o_stored_syndrome, io_numcode,
                                   *m_pllrProvider.get(), source);

    if (io_numcode == next_test_code) {
      this->decode(decoded, io_numcode, o_stored_syndrome);

      if (testConvergence(decoded, source)) {
        return 0;
      }

      next_test_code = std::min(next_test_code + step, numcodeMax - 1);

      if (!m_pllrProvider->doping_ready() &&
          m_matrix_provider->is_doping(next_test_code)) {
        m_pllrProvider->init_doping();
        // m_pdecoder->get_llr_order(io_numcode, m_pllrProvider->llr_order());
      }
    }
  }

  io_numcode = numcodeMax - 1;
  return errorCount(decoded, source);
}

template <class Matrix>
void InteractiveCoder<Matrix>::decode(BitVec &o_decoded, NumcodeIdx i,
                                      const Syndrome &stored_syndrome) {
  m_psyndromeMgr->compute_deacc(*m_decodage_syndrome.get(), stored_syndrome,
                                m_matrix_provider->getNumcode(i),
                                *m_pllrProvider.get());
  m_pdecoder->decode(o_decoded, m_psyndromeMgr->get_n(), i,
                     m_pllrProvider->llr(), *m_decodage_syndrome.get(),
                     m_matrix_provider->getMatrix(i));
}

template <class Matrix>
void InteractiveCoder<Matrix>::decode_symboles(
    BitSymbolProvider &io_bsp, const NumcodeIdxVec &numcodes,
    const SyndromesVec &stored_synd) {

  ICOV_ASSERT_MSG(numcodes.size() == stored_synd.size(),
                  "Rates vector and Syndrome vector must be same length");

  io_bsp.reset_src();
  int lx = stored_synd.size();

  ICOV_ASSERT_MSG(lx == io_bsp.bitplane_count(), "Bitplanes count must match");

  size_t n = get_n();

  ICOV_ASSERT_MSG(n == io_bsp.code_length(), "Code size must match");

  if (m_decoded_bits.size() != n)
    m_decoded_bits.resize(n);

  std::fill(m_decoded_bits.begin(), m_decoded_bits.end(), 0);

  for (int i = 0; i < lx; ++i) {
    this->decode_bitplane(m_decoded_bits, i, io_bsp, numcodes[i],
                          stored_synd[i]);
  }
}

template <class Matrix>
void InteractiveCoder<Matrix>::decode_bitplane(BitVec &output, int bp_idx,
                                               BitSymbolProvider &bsp,
                                               const NumcodeIdx &numcode,
                                               const Syndrome &stored_synd) {

  ICOV_TRACE << "DECODE BP " << bp_idx;

  // retrieve the type of given syndrome
  SyndromeType s_type = this->getSyndromeType(numcode);

  if (output.size() != get_n())
    output.resize(get_n());

  if (s_type != SyndromeType::SyndromeType_SOURCE) {
    icov::assert::equal(output.size(), m_pllrProvider->llr().size(),
                        ICOV_DEBUG_HEADER("Size does not match"));

    init_LLR_bitplane(bsp, bp_idx, bsp.src_bits()[0], bsp.src_symboles());
  }

  switch (s_type) {
  case IInteractiveCoder::SyndromeType_SOURCE:
    // Syndrome is directly equal to the source information
    ICOV_TRACE << "DECODE: SOURCE";
    icov::assert::equal(stored_synd.size(), get_n(),
                        ICOV_DEBUG_HEADER("Size does not match"));

    output.assign(stored_synd.data(), stored_synd.data() + get_n());

    //    for (size_t i = 0; i < get_n(); ++i) {
    //      output[i] = stored_synd[i];
    //    }
    break;

  case IInteractiveCoder::SyndromeType_NO_CODING:
    // No syndrome is needed, the LLR gives all the information to retrieve the
    // source
    ICOV_TRACE << "DECODE: NO CODING";

    ICOV_WARNING_MSG(stored_synd.size() == 0, "Syndrome should be empty");

    //    for (size_t i = 0; i < get_n(); ++i) {
    //      output[i] = m_pllrProvider->llr()[i] < 0 ? 1 : 0;
    //    }

    std::transform(m_pllrProvider->llr().cbegin(), m_pllrProvider->llr().cend(),
                   output.begin(), [](icov::LLRVal l) -> icov::BitVal {
                     return l < 0 ? icov::bit_t::one : icov::bit_t::zero;
                   });
    break;

  case IInteractiveCoder::SyndromeType_NORMAL:
    this->decode(output, this->getNumcodeIdx(numcode), stored_synd);
    break;

  default:
    ICOV_THROW_LOGICAL_EXCEPTION("Unmanaged case");
    break;
  }

  // inject the bitplane into the output symbole vector
  bsp.injectBitplane(bp_idx, output);
}

template <class Matrix>
void InteractiveCoder<Matrix>::init_LLR_bitplane(const BitSymbolProvider &bsp,
                                                 int bitplane_idx,
                                                 const BitVec &signs,
                                                 const SymboleVec &predec) {

  // init the LLR values
  if (bitplane_idx == 0) {
    // get LLR for first bitplane level i.e. sign
    m_pllrProvider->initLLR_s(bsp.distribution(), bsp.si_symboles());
  } else {
    // get LLR for bitplane after the sign
    m_pllrProvider->initLLR_b(bitplane_idx - 1, bsp.distribution(),
                              bsp.si_symboles(), signs, predec);
  }
}

template <class Matrix>
void InteractiveCoder<Matrix>::encode_symboles(
    std::vector<NumcodeIdxVec> &numcodesIdx,
    std::vector<SyndromesVec> &syndromes, const std::vector<SIVec> &si,
    const QaryDistribution::quantization_params &p, const SymboleVec &src) {

  // init outputs
  if (numcodesIdx.size() != si.size()) {
    numcodesIdx.resize(si.size(), NumcodeIdxVec());
  }

  if (syndromes.size() != si.size()) {
    syndromes.resize(si.size(), SyndromesVec());
  }

  std::vector<QaryDistribution> sipz(si.size());

  // symboles to bitplanes

  // encode with each SI
  for (size_t i = 0; i < si.size(); i++) {
    ICOV_INFO << "SI=" << i + 1 << "/" << si.size();
    BitSymbolProvider bsp(src);
    bsp.set_si_and_compute_pz_params(si[i], p);
    sipz[i] = bsp.distribution();
    this->encode_bitplanes(bsp, numcodesIdx[i], syndromes[i]);
  }

  // TEST DECODE WITH WORST SI for EACH BITPLANE
  BitSymbolProvider bsp(src);
  std::vector<Syndrome> worst_syndromes(bsp.bitplane_count());
  for (int i = 0; i < bsp.bitplane_count(); ++i) {
    ICOV_INFO << "TEST DECODING BITPLANE " << i;
    NumcodeIdx worst_rate = 0;
    worst_syndromes[i] = syndromes[0][i];

    // Now find worst SI
    for (int k = 0; k < (int)numcodesIdx.size(); ++k) {
      NumcodeIdx rate_k = numcodesIdx[k][i];
      ICOV_INFO << "SI has rate " << rate_k;
      if (worst_rate < rate_k) {
        worst_rate = rate_k;
        worst_syndromes[i] = syndromes[k][i];
      }
    }
    ICOV_INFO << "Worst SI has rate " << worst_rate;
  }

  // Now decode EACH SI with ITS RATE and THE WORST SYNDROME
  for (int k = 0; k < (int)numcodesIdx.size(); ++k) {
    BitSymbolProvider bsp_dec(get_n());
    bsp_dec.set_si_and_qaryDistribution(si[k], sipz[k]);
    // NumcodeIdx rate_k = numcodesIdx[k][i];
    //    ICOV_INFO << "SI has rate " << rate_k;
    //    ICOV_DEBUG << "Resize syndrome " << get_m(rate_k);
    // worst_syndromes[i].resize(get_m(rate_k));
    //    BitVec x_dec(get_n(), 0);

    ICOV_DEBUG << "DECODE SI " << k;
    //    this->decode(x_dec, rate_k, worst_synd);

    //    if (x_dec != x)
    //      ICOV_ERROR << "ERROR";

    this->decode_symboles(bsp_dec, numcodesIdx[k], worst_syndromes);
    for (int i = 0; i < bsp.bitplane_count(); ++i) {
      ICOV_INFO << "CHECK BITPLANE " << i << " at rate "
                << get_r(numcodesIdx[k][i]) << " using syndrome from rate "
                << get_r(worst_syndromes[i].size(), get_n());
      if (bsp_dec.src_bits()[i] != bsp.src_bits()[i]) {
        ICOV_ERROR << "ERRORS DETECTED IN DECODED BITPLANE " << i;
      }
    }
  }
}

template <class Matrix>
void InteractiveCoder<Matrix>::encode_bitplanes(const BitSymbolProvider &bsp,
                                                NumcodeIdxVec &numcodeIdx,
                                                SyndromesVec &stored_synd) {

  auto syndromeMgr = this->syndromeMgr();
  int n = syndromeMgr->get_n();
  size_t lx = bsp.bitplane_count();

  numcodeIdx = NumcodeIdxVec(lx, 0);

  if (stored_synd.size() != lx) {
    stored_synd = SyndromesVec(lx, SyndromesVec::value_type(n));
  }
  SymboleVec predec(n, 0);
  for (size_t i = 0; i < lx; i++) {
    ICOV_TRACE << "ENCODE BP " << i;
    stored_synd[i].resize(n);
    stored_synd[i].fill(0);
    encode_bitplane(bsp, i, numcodeIdx[i], stored_synd[i], predec);
  }

  ICOV_ASSERT_MSG(predec == bsp.src_symboles(),
                  "Symbole values are not recovered from encoding process");
}

template <class Matrix>
void InteractiveCoder<Matrix>::encode_bitplane(const BitSymbolProvider &bsp,
                                               int bitplane_idx,
                                               NumcodeIdx &numcodeIdx,
                                               Syndrome &stored_synd,
                                               SymboleVec &predec) {

  auto m_coder = this;
  auto syndromeMgr = m_coder->syndromeMgr();
  const auto &src_symbols = bsp.src_symboles();
  const auto &src_bits = bsp.src_bits();
  const auto &bitplane_bits = src_bits.at(bitplane_idx);

  const auto &sign_bits = src_bits[0];

  int n = syndromeMgr->get_n();
  int lx = bsp.bitplane_count();
  int shift = (lx - bitplane_idx - 1);

  //    syndrome accumulation
  syndromeMgr->init_accumulation(bitplane_bits);

  init_LLR_bitplane(bsp, bitplane_idx, sign_bits, predec);

  const NumcodeIdx no_coding_flag = SyndromeType_NO_CODING;
  const NumcodeIdx coding_flag = SyndromeType_NORMAL;
  NumcodeIdx coding_offset = SyndromeType_SOURCE;
  numcodeIdx = 0;

  // TO CODE OR NOT TO CODE ???
  coding_offset = std::equal(m_pllrProvider->llr().begin(),
                             m_pllrProvider->llr().end(), bitplane_bits.begin(),
                             [](LLRVal llr, BitVal srcBit) {
                               return (llr > 0 && srcBit == bit_t::zero) ||
                                      (llr < 0 && srcBit == bit_t::one);
                             })
                      ? no_coding_flag
                      : coding_flag;

  if (coding_offset == coding_flag) {
    try {
      int numerrors = this->encode(numcodeIdx, stored_synd, bitplane_bits);
      if (numerrors != 0)
        ICOV_THROW_LOGICAL_EXCEPTION("No convergence for bitplane " +
                                     std::to_string(bitplane_idx));

      if (params.CheckBitplaneEncoding) {
        ICOV_TRACE << "CHECK SYNDROME BP " << bitplane_idx;
        BitVec decoded;
        decode(decoded, numcodeIdx, stored_synd);
        icov::assert::check(decoded == src_bits[bitplane_idx],
                            "INTERNAL Decoding BP check failed " +
                                std::to_string(bitplane_idx));
      }

    } catch (const std::exception &e) {
      numcodeIdx = 0;
      coding_offset = SyndromeType_SOURCE;

      stored_synd.fillFrom(bitplane_bits);
      ICOV_WARNING << "Bitplane " << bitplane_idx;
      ICOV_WARNING << "Encoding bitplane as source as we catched exception in "
                      "bitplane encoding: "
                   << e.what();
    } catch (...) {
      numcodeIdx = 0;
      coding_offset = SyndromeType_SOURCE;

      stored_synd.fillFrom(bitplane_bits);
      ICOV_WARNING << "Bitplane " << bitplane_idx;
      ICOV_WARNING
          << "Encoding bitplane as source as we catched unknown exception in "
             "bitplane encoding";
    }

  } else {
    stored_synd.resize(0);
    ICOV_TRACE << "ENCODE: NO CODING";
  }

  predec.resize(n);

  std::transform(src_symbols.cbegin(), src_symbols.cend(), predec.begin(),
                 [&](int srcSymbol) {
                   if (bitplane_idx == 0)
                     return 0;
                   if (shift == 0)
                     return srcSymbol;
                   return std::abs(srcSymbol) >> shift;
                 });

  numcodeIdx += coding_offset;
}
} // namespace ldpc_codec_layer
} // namespace icov
