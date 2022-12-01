
#include "Aff3ctDecoder.h"

//#include <chrono>

icov::Aff3ctDecoder::Aff3ctDecoder(int n_dec)
    : IDecoder(), m_decvector(n_dec) {}

void icov::Aff3ctDecoder::decode(BitVec &o_decoded, int n, int idx,
                                 const std::vector<LLRVal> &llr,
                                 Syndrome &syndrome, const AffMatrix &matrix) {
  // std::cout << "decode" << std::endl;

  if ((int)o_decoded.size() != n) {
    o_decoded = BitVec(n, 0);
  }
  int d = std::min(idx, (int)m_decvector.size() - 1);
  m_decvector[d]->reset();
  m_decvector[d]->setSyndrome(&syndrome);
  m_decvector[d]->decode_siho(llr, o_decoded);
}

// void icov::Aff3ctDecoder::decode_doping(BitVec &o_decoded,
// std::vector<size_t> &o_doping_idx, int n, const std::vector<LLRVal> &llr,
// Syndrome &syndrome, const AffMatrix &matrix)
//{
//     auto affdec = AffLDPCA(n, 100, matrix);
//     affdec.setSyndrome(&syndrome);
//     affdec.decode_siho(llr, o_decoded);
//     affdec.get_llr_order(o_doping_idx);
//}

void icov::Aff3ctDecoder::init_decoder(int i, int n, int n_ite,
                                       const AffMatrix &matrix) {
  if (i >= (int)m_decvector.size())
    m_decvector.resize(i + 1);
  m_decvector[i] = std::unique_ptr<AffLDPCA>(new AffLDPCA(n, n_ite, matrix));
}

void icov::Aff3ctDecoder::get_llr_order(int i_numcode,
                                        std::vector<size_t> &o_idx) const {
  int d = std::min(i_numcode, (int)m_decvector.size() - 1);
  m_decvector[d]->get_llr_order(o_idx);
}
