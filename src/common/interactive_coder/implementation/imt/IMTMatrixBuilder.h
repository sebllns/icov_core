#ifndef IMTMATRIXBUILDER_H
#define IMTMATRIXBUILDER_H

#include <IAff3ctMatrixBuilder.h>
#include "IMTSyndrome.h"

namespace icov {
class IMTMatrixBuilder : public IAff3ctMatrixBuilder
{
public:
    IMTMatrixBuilder(IMTTools* tools):
        m_tools(tools)
    {

    }
    ~IMTMatrixBuilder() = default;


    // IMatrixBuilder interface
public:
    AffMatrix *buildMatrix(int numcode) const override;

private:
    std::shared_ptr<IMTTools> m_tools;
};
}
#endif // IMTMATRIXBUILDER_H
