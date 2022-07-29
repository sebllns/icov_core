#ifndef IDECODER_H
#define IDECODER_H

#include "common.h"

namespace icov
{

template <class M>
class IDecoder
{
public:
    IDecoder() = default;
    virtual ~IDecoder() = default;

    virtual void decode(BitVec& o_decoded, int n, int numcode, const std::vector<LLRVal>& llr, Syndrome& syndrome, const M& matrix) = 0;
};
}
#endif // IDECODER_H
