#ifndef VARODAYANMatrixProvider_H
#define VARODAYANMatrixProvider_H

#include "interactive_coder/aff3ct/IAff3ctMatrixProvider.h"
#include "VarodayanSyndrome.h"

namespace icov {
class VarodayanMatrixProvider : public IAff3ctMatrixProvider
{
public:
    VarodayanMatrixProvider(VarodayanSyndromeManager* tools):
        m_tools(tools)
    {

    }
    ~VarodayanMatrixProvider() = default;


    // IMatrixProvider interface
protected:
    const AffMatrix _buildMatrix(int numcode) override;

private:
    std::shared_ptr<VarodayanSyndromeManager> m_tools;
};
}
#endif // VARODAYANMatrixProvider_H
