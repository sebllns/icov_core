#include "Aff3ctDecoder.h"

namespace icov {
namespace ldpc_codec_layer {
Aff3ctDecoder::Aff3ctDecoder(int n_dec) : ILDPC_Decoder(), m_decvector(n_dec) {}

void Aff3ctDecoder::decode(BitVec &o_decoded, int n, int idx,
                                 const std::vector<LLRVal> &llr,
                                 Syndrome &syndrome, const AffMatrix &matrix) {
  if ((int)o_decoded.size() != n) {
    o_decoded = BitVec(n, 0);
  }
  int d = std::min(idx, (int)m_decvector.size() - 1);
  m_decvector[d]->reset();
  m_decvector[d]->setSyndrome(&syndrome);
  m_decvector[d]->decode_siho(llr, o_decoded);
}

void Aff3ctDecoder::init_decoder(int i, int n, int n_ite,
                                 const AffMatrix &matrix) {
  if (i >= (int)m_decvector.size())
    m_decvector.resize(i + 1);
  m_decvector[i] = std::make_unique<AffLDPCA>(n, n_ite, matrix);
}

void Aff3ctDecoder::get_llr_order(int i_numcode,
                                        std::vector<size_t> &o_idx) const {
  int d = std::min(i_numcode, (int)m_decvector.size() - 1);
  m_decvector[d]->get_llr_order(o_idx);
}
} // namespace ldpc_codec_layer
} // namespace icov
