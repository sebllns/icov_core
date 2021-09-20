#ifndef AFF3CTDECODER_H
#define AFF3CTDECODER_H

#include "interactive_coder/interface/IDecoder.h"

#include <aff3ct.hpp>
#include "Decoder_LDPCA_BP_flooding_SPA.hpp"

using namespace aff3ct;

using AffMatrix = tools::Sparse_matrix;
using AffLDPCA = module::Decoder_LDPCA_BP_flooding_SPA<icov::CodedVal, icov::LLRVal>;

namespace icov
{
class Aff3ctDecoder : public IDecoder<AffMatrix>
{
public:
    Aff3ctDecoder(int n_dec);

    void decode(std::vector<CodedVal>& o_decoded, int n, int numcode,  int n_ite, const std::vector<LLRVal>& llr, Syndrome& syndrome, const AffMatrix& matrix)
    override;

    void decode_doping(std::vector<CodedVal>& o_decoded, std::vector<size_t> &o_doping_idx, int n, int numcode,  int n_ite, const std::vector<LLRVal>& llr, Syndrome& syndrome, const AffMatrix& matrix);

    void init_decoder(int numcode, int n,  int n_ite, const AffMatrix& matrix);

protected:
    std::vector<std::unique_ptr<AffLDPCA>> m_decvector;
};

}
#endif // AFF3CTDECODER_H
