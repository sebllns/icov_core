#ifndef AFF3CTDECODER_H
#define AFF3CTDECODER_H

#include "interactive_coder/interface/IDecoder.h"

#include <aff3ct.hpp>

using namespace aff3ct;

using AffMatrix = tools::Sparse_matrix;

namespace icov
{
class Aff3ctDecoder : public IDecoder<AffMatrix>
{
public:
    Aff3ctDecoder();

    void decode(std::vector<CodedVal>& o_decoded, int n, int numcode,  int n_ite, const std::vector<SIVal>& llr, Syndrome& syndrome, const AffMatrix& matrix)
    override;


    void decode_doping(std::vector<CodedVal>& o_decoded, std::vector<size_t> &o_doping_idx, int n, int numcode,  int n_ite, const std::vector<SIVal>& llr, Syndrome& syndrome, const AffMatrix& matrix);
};

}
#endif // AFF3CTDECODER_H
