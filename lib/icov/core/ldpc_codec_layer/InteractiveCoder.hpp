#pragma once

#include "InteractiveCoder.h"

#include "FastArray.h"
#include "icov_common.h"
#include "icov_log.h"
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
int InteractiveCoder<Matrix>::encode(NumcodeIdx &i, Syndrome &o_stored_syndrome,
                                     const BitVec &source) {
  o_stored_syndrome.resize(0);

  BitVec decoded(m_psyndromeMgr->get_n());

  bool convergence = false;
  while (i < m_matrix_provider->getNumCodesCount() && !convergence) {
    uint32_t io_numcode = m_matrix_provider->getNumcode(i);

#if ICOV_CORE_DEBUG_TRACE_ENABLED
    ICOV_TRACE << "m = " << this->syndromeMgr()->get_m(io_numcode)
               << ", r = " << this->get_r(i);
#endif
    m_psyndromeMgr->compute_stored(o_stored_syndrome, io_numcode,
                                   *m_pllrProvider.get(), source);
    this->decode(decoded, i, o_stored_syndrome);

    convergence = testConvergence(decoded, source);
    if (!convergence) {
      ++i;
      if (!m_pllrProvider->doping_ready() && m_matrix_provider->is_doping(i)) {
        this->m_pllrProvider->init_doping();
        // m_pdecoder->get_llr_order(io_numcode, m_pllrProvider->llr_order());
      }
    }
  }

  if (convergence)
    return 0;
  return errorCount(decoded, source);
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
void InteractiveCoder<Matrix>::decode(BitVec &o_decoded, uint32_t i,
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

  if (output.size() != get_n())
    output.resize(get_n());

  if (numcode != 1)
    init_LLR_bitplane(bsp, bp_idx, bsp.src_bits()[0], bsp.src_symboles());

  if (numcode > 1) {
    this->decode(output, numcode - 2, stored_synd);
  } else if (numcode == 1) {
    ICOV_TRACE << "DECODE: SOURCE";
#if ICOV_ASSERTION_ENABLED
    icov::assert::equal(stored_synd.size(), get_n(), "Size does not match");
#endif

    output.assign(stored_synd.data(), stored_synd.data() + get_n());

    //    for (size_t i = 0; i < get_n(); ++i) {
    //      output[i] = stored_synd[i];
    //    }
  } else if (numcode == 0) {
    ICOV_TRACE << "DECODE: NO CODING";

#if ICOV_ASSERTION_ENABLED
    icov::assert::equal(output.size(), m_pllrProvider->llr().size(),
                        "Size does not match");
#endif

    //    for (size_t i = 0; i < get_n(); ++i) {
    //      output[i] = m_pllrProvider->llr()[i] < 0 ? 1 : 0;
    //    }

    std::transform(m_pllrProvider->llr().cbegin(), m_pllrProvider->llr().cend(),
                   output.begin(), [](icov::LLRVal l) -> icov::BitVal {
                     return l < 0 ? 1 : 0;
                   });
  }

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

  const NumcodeIdx no_coding_flag = 0;
  const NumcodeIdx coding_flag = 1;
  const NumcodeIdx coding_offset = coding_flag + 1;

  // TO CODE OR NOT TO CODE ???
  numcodeIdx = std::equal(m_pllrProvider->llr().begin(),
                          m_pllrProvider->llr().end(), bitplane_bits.begin(),
                          [](LLRVal llr, BitVal srcBit) {
                            return (llr > 0 && srcBit == bit_t::zero) ||
                                   (llr < 0 && srcBit == bit_t::one);
                          })
                   ? no_coding_flag
                   : coding_flag;

  if (numcodeIdx == coding_flag) {
    try {
      int numerrors = this->encode(numcodeIdx, stored_synd, bitplane_bits);
      icov::assert::check(numerrors == 0, "No convergence for bitplane " +
                                              std::to_string(bitplane_idx));

      if (params.CheckBitplaneEncoding) {
        ICOV_TRACE << "CHECK SYNDROME BP " << bitplane_idx;
        BitVec decoded;
        decode(decoded, numcodeIdx, stored_synd);
        icov::assert::check(decoded == src_bits[bitplane_idx],
                            "INTERNAL Decoding BP check failed " +
                                std::to_string(bitplane_idx));
      }

      numcodeIdx += coding_offset;
    } catch (const std::exception &e) {
      numcodeIdx = 1;
      stored_synd.fillFrom(bitplane_bits);
      ICOV_WARNING << "Bitplane " << bitplane_idx;
      ICOV_WARNING << "Encoding bitplane as source as we catched exception in "
                      "bitplane encoding: "
                   << e.what();
      ICOV_ERROR << boost::current_exception_diagnostic_information();
    } catch (...) {
      numcodeIdx = 1;
      stored_synd.fillFrom(bitplane_bits);
      ICOV_WARNING << "Bitplane " << bitplane_idx;
      ICOV_WARNING
          << "Encoding bitplane as source as we catched unknown exception in "
             "bitplane encoding";
      ICOV_ERROR << boost::current_exception_diagnostic_information();
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
}
} // namespace ldpc_codec_layer
} // namespace icov
