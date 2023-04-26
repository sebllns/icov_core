#pragma once

#include "Decoder_LDPCA_BP_flooding_SPA.hpp"
#include "Tools/Algo/Matrix/Sparse_matrix/Sparse_matrix.hpp"
#include "ldpc_codec_layer/interface/ILDPC_Decoder.h"

namespace icov {
namespace ldpc_codec_layer {

using AffMatrix = aff3ct::tools::Sparse_matrix;
using AffLDPCA = Decoder_LDPCA_BP_flooding_SPA<icov::BitVal, icov::LLRVal>;

class Aff3ctDecoder : public ILDPC_Decoder<AffMatrix> {
public:
  Aff3ctDecoder(int n_dec);
  ~Aff3ctDecoder() = default;

  void decode(BitVec &o_decoded, int n, int idx, const std::vector<LLRVal> &llr,
              Syndrome &syndrome, const AffMatrix &matrix) override;

  void get_llr_order(int i_numcode, std::vector<size_t> &o_idx) const override;

  void init_decoder(int i, int n, int n_ite, const AffMatrix &matrix);

protected:
  std::vector<std::unique_ptr<AffLDPCA>> m_decvector;
};
} // namespace ldpc_codec_layer
} // namespace icov
