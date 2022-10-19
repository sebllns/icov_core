#include "InteractiveCoder.h"
#include "helpers.h"

using namespace icov;

template <class Matrix>
bool InteractiveCoder<Matrix>::testConvergence(const BitVec &decoded,
                                               const BitVec &source) {
  if (decoded.size() != source.size())
    return false;

  for (int i = 0; i < decoded.size(); i++) {
    if (decoded[i] != source[i])
      return false;
  }

  return true;
}

template <class Matrix>
int InteractiveCoder<Matrix>::errorCount(const BitVec &decoded,
                                         const BitVec &source) {
  float numerrors = 0;
  for (int i = 0; i < decoded.size(); i++) {
    numerrors += std::abs(decoded[i] - source[i]);
  }

  return (int)numerrors;
}

template <class Matrix>
int InteractiveCoder<Matrix>::encode(uint32_t &i, Syndrome &o_stored_syndrome,
                                     const BitVec &source) {
  o_stored_syndrome.resize(0);

  BitVec decoded(m_psyndromeMgr->get_n());

  bool convergence = false;
  while (i < m_matrix_provider->getNumCodesCount() && !convergence) {
    uint32_t io_numcode = m_matrix_provider->getNumcode(i);

    //        std::cout << ">> m=" << m_psyndromeMgr->get_m(io_numcode) <<
    //        std::endl;

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
  int n = source.size();

  BitVec decoded(m_psyndromeMgr->get_n());

  uint32_t next_test_code = io_numcode;

  for (io_numcode = 0; io_numcode < numcodeMax; io_numcode++) {
    // std::cout << io_numcode << std::endl;

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
void InteractiveCoder<Matrix>::encode(std::vector<SIRates> &rates,
                                      std::vector<SISyndromes> &syndromes,
                                      const std::vector<SIVec> &si,
                                      const BitSymbolProvider::params &p,
                                      const SymboleVec &src) {

  if (rates.size() != si.size()) {
    rates.resize(si.size(), SIRates());
  }

  if (syndromes.size() != si.size()) {
    syndromes.resize(si.size(), SISyndromes());
  }

  BitSymbolProvider bsp(src);

  for (size_t i = 0; i < si.size(); i++) {
    std::cout << "si=" << i + 1 << "/" << si.size() << std::endl;
    bsp.set_si(si[i], p);
    this->encode_bitplanes(bsp, rates[i], syndromes[i]);
  }
}

template <class Matrix>
void InteractiveCoder<Matrix>::encode_bitplanes(const BitSymbolProvider &bsp,
                                                SIRates &rate,
                                                SISyndromes &stored_synd) {

  auto syndromeMgr = this->syndromeMgr();
  int n = syndromeMgr->get_n();
  size_t lx = bsp.x_binary_length();

  //  std::cout << "call 01" << std::endl;

  rate = std::vector<Rate>(lx, 0.0);

  if (stored_synd.size() != lx) {
    stored_synd = std::vector<Syndrome>(lx, Syndrome(n));
  }

  //  std::cout << "call 02" << std::endl;

  SymboleVec predec(n, 0);
  for (size_t i = 0; i < lx; i++) {
    stored_synd[i].resize(n);
    stored_synd[i].fill(0);
    encode_bitplane(bsp, i, rate[i], stored_synd[i], predec);
  }

  //  std::cout << "call 03" << std::endl;

  ICOV_ASSERT_MSG(predec == bsp.src_symboles(),
                  "Symbole values are not recovered from encoding process");
}

template <class Matrix>
void InteractiveCoder<Matrix>::encode_bitplane(const BitSymbolProvider &bsp,
                                               int bitplane_idx, Rate &rate,
                                               Syndrome &stored_synd,
                                               SymboleVec &predec) {

  //  std::cout << "call" << std::endl;

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

  //  std::cout << "prepare" << std::endl;

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

  //  std::cout << "encode" << std::endl;

  uint numcode = 0;
  int numerrors = m_coder->encode(numcode, stored_synd, src_bits[bitplane_idx]);

  //  std::cout << "done" << std::endl;

  rate = (((double)syndromeMgr->get_m(matrixPvdr->getNumcode(numcode))) /
          ((double)n));
  // reset doping state and LLR
  syndromeMgr->reset_doping();
  for (int l = 0; l < n; l++) {
    llrProvider->operator[](l) = llr[l];
  }

  //  std::cout << "end" << std::endl;

  // TODO handle error
  if (numerrors != 0) {
    std::cerr << "ERROR: No convergence for bitplane=" << bitplane_idx
              << std::endl;
  } else {
    std::cout << "b=" << bitplane_idx << " rate=" << rate << std::endl;
    // check_decoding(i);
  }

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
