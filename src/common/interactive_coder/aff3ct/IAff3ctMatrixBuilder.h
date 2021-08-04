#ifndef IAFF3CTMATRIXBUILDER_H
#define IAFF3CTMATRIXBUILDER_H

#include <IMatrixBuilder.h>
#include <aff3ct.hpp>

using namespace aff3ct;

using AffMatrix = tools::Sparse_matrix;

namespace icov {

    class IAff3ctMatrixBuilder : public IMatrixBuilder<AffMatrix>
    {
    public:
        IAff3ctMatrixBuilder() = default;
        virtual ~IAff3ctMatrixBuilder() = default;

        // IMatrixBuilder interface
    public:
        virtual AffMatrix *buildMatrix(int numcode) const = 0;
    };
}

#endif // IAFF3CTMATRIXBUILDER_H
