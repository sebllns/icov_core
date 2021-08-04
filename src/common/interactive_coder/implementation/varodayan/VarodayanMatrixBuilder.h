#ifndef VARODAYANMATRIXBUILDER_H
#define VARODAYANMATRIXBUILDER_H

#include <IAff3ctMatrixBuilder.h>
#include "VarodayanSyndrome.h"

namespace icov {
class VarodayanMatrixBuilder : public IAff3ctMatrixBuilder
{
public:
    VarodayanMatrixBuilder(const VarodayanTools& tools):
        m_tools(tools)
    {

    }
    ~VarodayanMatrixBuilder() = default;


    // IMatrixBuilder interface
public:
    AffMatrix *buildMatrix(int numcode) const override;

private:
    const VarodayanTools& m_tools;
};
}
#endif // VARODAYANMATRIXBUILDER_H
