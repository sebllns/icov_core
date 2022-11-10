#include "InteractiveCoder.h"
#include "helpers.h"

using namespace icov;

template <class Matrix> size_t InteractiveCoder<Matrix>::get_n() const {
  return m_psyndromeMgr->get_n();
}

template <class Matrix>
Rate InteractiveCoder<Matrix>::getRate(const NumcodeIdx &numcodeIdx) const {
  return static_cast<Rate>(
             m_psyndromeMgr->get_m(m_matrix_provider->getNumcode(numcodeIdx))) /
         static_cast<Rate>(get_n());
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
    std::cout << io_numcode << std::endl;
    m_psyndromeMgr->compute_stored(o_stored_syndrome, io_numcode,
                                   *m_pllrProvider.get());
    this->decode(decoded, i, o_stored_syndrome);

    convergence = testConvergence(decoded, source);
    if (!convergence)
      ++i;
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
                                   *m_pllrProvider.get());

    if (io_numcode == next_test_code) {
      this->decode(decoded, io_numcode, o_stored_syndrome);

      if (testConvergence(decoded, source)) {
        return 0;
      }

      next_test_code = std::min(next_test_code + step, numcodeMax - 1);
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
void InteractiveCoder<Matrix>::decode_bitplanes(
    SymboleVec &output, const SINumcodeIdx &numcodes,
    const SISyndromes &stored_synd) {

  ICOV_ASSERT_MSG(numcodes.size() == stored_synd.size(),
                  "Rates vector and Syndrome vector must be same length");

  size_t n = get_n();
  output.resize(n);
  m_decoded_bits.resize(n);

  std::fill(output.begin(), output.end(), 0);
  std::fill(m_decoded_bits.begin(), m_decoded_bits.end(), 0);

  int bp = stored_synd.size();
  for (int i = bp - 1; i >= 0; --i) {

    this->decode(m_decoded_bits, numcodes[i], stored_synd[i]);
    for (size_t k = 0; k < n; k++) {
      if (m_decoded_bits[k]) {
        if (i == 0) {
          output[k] *= -1;
        } else {
          output[k] += (1 << i);
        }
      }
    }
  }
}

template <class Matrix>
void InteractiveCoder<Matrix>::init_LLR_bitplane(BitSymbolProvider bsp,
                                                 int bitplane_idx) {

  //  int predec = 0;

  //  // init the LLR values
  //  if (bitplane_idx == 0) {
  //    // get LLR for first bitplane level i.e. sign
  //    m_pllrProvider->initLLR_s(bsp, bsp.si_symboles(), bsp.src_bits()[0]);
  //  } else {
  //    // get LLR for bitplane
  //    m_pllrProvider->initLLR_b(bitplane_idx - 1, bsp, bsp.si_symboles(),
  //                              bsp.src_bits()[0], predec, bsp.src_bits()[0]);
  //  }
}

template <class Matrix>
void InteractiveCoder<Matrix>::encode(std::vector<SINumcodeIdx> &numcodesIdx,
                                      std::vector<SISyndromes> &syndromes,
                                      const std::vector<SIVec> &si,
                                      const BitSymbolProvider::params &p,
                                      const SymboleVec &src) {

  if (numcodesIdx.size() != si.size()) {
    numcodesIdx.resize(si.size(), SINumcodeIdx());
  }

  if (syndromes.size() != si.size()) {
    syndromes.resize(si.size(), SISyndromes());
  }

  BitSymbolProvider bsp(src);

  for (size_t i = 0; i < si.size(); i++) {
    std::cout << "si=" << i + 1 << "/" << si.size() << std::endl;
    bsp.set_si(si[i], p);
    this->encode_bitplanes(bsp, numcodesIdx[i], syndromes[i]);
  }
}

template <class Matrix>
void InteractiveCoder<Matrix>::encode_bitplanes(const BitSymbolProvider &bsp,
                                                SINumcodeIdx &numcodeIdx,
                                                SISyndromes &stored_synd) {

  auto syndromeMgr = this->syndromeMgr();
  int n = syndromeMgr->get_n();
  size_t lx = bsp.x_binary_length();

  numcodeIdx = SINumcodeIdx(lx, 0);

  if (stored_synd.size() != lx) {
    stored_synd = SISyndromes(lx, SISyndromes::value_type(n));
  }
  SymboleVec predec(n, 0);
  for (size_t i = 0; i < lx; i++) {
    std::cout << "ENCODE BP " << i << std::endl;
    stored_synd[i].resize(n);
    stored_synd[i].fill(0);
    encode_bitplane(bsp, i, numcodeIdx[i], stored_synd[i], predec);
    this->m_pllrProvider->reset();

    //    int k = i;
    //    std::cout << "POST CHECK SYNDROME BP " << k << std::endl;
    //    BitVec decoded;
    //    decode(decoded, numcodeIdx[k], stored_synd[k]);
    //    icov::helpers::assert::check(decoded == bsp.src_bits()[k],
    //                                 " POST INTERNAL Decoding BP check failed
    //                                 " +
    //                                     std::to_string(k));

    //    std::cout << "NUMCODE " << numcodeIdx[k] << std::endl;
    //    std::cout << "SYND " << stored_synd[k].to_string(" ") << std::endl;
    //    std::cout << "SOURCE [";
    //    icov::helpers::io::write_vector(std::cout, bsp.src_bits()[k], ' ')
    //        << "]" << std::endl;
    //    std::cout << "DECODED [";
    //    icov::helpers::io::write_vector(std::cout, decoded, ' ')
    //        << "]" << std::endl;
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
  auto llrProvider = m_coder->llrProvider();
  auto syndromeMgr = m_coder->syndromeMgr();
  auto matrixPvdr = m_coder->matrixProvider();
  auto src_symbols = bsp.src_symboles();
  auto src_bits = bsp.src_bits();
  auto side_info = bsp.si_symboles();

  int n = syndromeMgr->get_n();
  int lx = bsp.x_binary_length();
  int bitplane = (lx - bitplane_idx - 1);

  //    syndrome accumulation
  syndromeMgr->init_accumulation(src_bits[bitplane_idx]);

  if (bitplane_idx == 0) {
    // get LLR for first bitplane level i.e. sign
    llrProvider->initLLR_s(bsp, side_info, src_bits[bitplane_idx]);
  } else {
    // get LLR for bitplane
    llrProvider->initLLR_b(bitplane_idx - 1, bsp, side_info, src_bits[0],
                           predec, src_bits[bitplane_idx]);
  }

  // save LLR (in case of doping)
  auto llr = llrProvider->llr();

  numcodeIdx = 0;
  int numerrors =
      m_coder->encode(numcodeIdx, stored_synd, src_bits[bitplane_idx]);

  // reset doping state and LLR
  syndromeMgr->reset_doping();
  for (int l = 0; l < n; l++) {
    llrProvider->operator[](l) = llr[l];
  }

  std::cout << "CHECK SYNDROME BP " << bitplane_idx << std::endl;
  BitVec decoded;
  decode(decoded, numcodeIdx, stored_synd);
  icov::helpers::assert::check(decoded == src_bits[bitplane_idx],
                               "INTERNAL Decoding BP check failed " +
                                   std::to_string(bitplane_idx));

  // TODO handle error
  if (numerrors != 0) {
    std::cerr << "ERROR: No convergence for bitplane=" << bitplane_idx
              << std::endl;
  } /*else {
    std::cout << "b=" << bitplane_idx << " rate=" << rate << std::endl;
    // check_decoding(i);
  }*/

  if (bitplane_idx > 0) {
    // fill predecoded bit values
    for (int k = 0; k < n; k++) {
      predec[k] = std::abs(src_symbols[k]) >> bitplane;
      if (bitplane_idx == (lx - 1)) {
        predec[k] *= (1 - 2 * src_bits[0][k]);
        icov::helpers::assert::check(predec[k] == src_symbols[k],
                                     "Expected " +
                                         std::to_string(src_symbols[k]) +
                                         " got " + std::to_string(predec[k]));
      }
    }
  }
}
