#ifndef IAFF3CTMatrixProvider_H
#define IAFF3CTMatrixProvider_H

#include "interactive_coder/interface/IMatrixProvider.h"
#include <aff3ct.hpp>

using namespace aff3ct;

using AffMatrix = tools::Sparse_matrix;

namespace icov {

    class IAff3ctMatrixProvider : public IMatrixProvider<AffMatrix>
    {
    public:
        IAff3ctMatrixProvider() = default;
        virtual ~IAff3ctMatrixProvider() = default;

        // IMatrixProvider interface
    public:
        virtual AffMatrix *buildMatrix(int numcode) const = 0;
    };
}

#endif // IAFF3CTMatrixProvider_H
