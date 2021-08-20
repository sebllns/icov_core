#ifndef VARODAYANMATRIXBUILDER_H
#define VARODAYANMATRIXBUILDER_H

#include "interactive_coder/aff3ct/IAff3ctMatrixBuilder.h"
#include "VarodayanSyndrome.h"

namespace icov {
class VarodayanMatrixBuilder : public IAff3ctMatrixBuilder
{
public:
    VarodayanMatrixBuilder(VarodayanSyndromeManager* tools):
        m_tools(tools)
    {

    }
    ~VarodayanMatrixBuilder() = default;


    // IMatrixBuilder interface
public:
    AffMatrix *buildMatrix(int numcode) const override;

private:
    std::shared_ptr<VarodayanSyndromeManager> m_tools;
};
}
#endif // VARODAYANMATRIXBUILDER_H
