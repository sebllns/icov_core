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

    virtual void decode(std::vector<CodedVal>& o_decoded, int n, int numcode,  int n_ite, const std::vector<SIVal>& side_info, Syndrome& syndrome, const M& matrix) = 0;
};
}
#endif // IDECODER_H
