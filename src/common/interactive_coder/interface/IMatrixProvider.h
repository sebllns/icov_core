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
    virtual const M& buildMatrix(int numcode) = 0;
    virtual const M& getMatrix(int numcode) const = 0;
    virtual bool hasMatrix(int numCode) const = 0;
};
}
#endif // IMatrixProvider_H
