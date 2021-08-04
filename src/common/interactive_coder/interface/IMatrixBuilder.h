#ifndef IMATRIXBUILDER_H
#define IMATRIXBUILDER_H


#include "common_types.h"

namespace icov {

template <class M>
class IMatrixBuilder
{
public:
    IMatrixBuilder() = default;
    virtual ~IMatrixBuilder() = default;
    virtual  M* buildMatrix(int numcode) const = 0;
};
}
#endif // IMATRIXBUILDER_H
