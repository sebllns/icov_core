#ifndef IMatrixProvider_H
#define IMatrixProvider_H


#include "common.h"

namespace icov {

template <class M>
class IMatrixProvider
{
public:
    IMatrixProvider() = default;
    virtual ~IMatrixProvider() = default;
    virtual  M* buildMatrix(int numcode) const = 0;
};
}
#endif // IMatrixProvider_H
