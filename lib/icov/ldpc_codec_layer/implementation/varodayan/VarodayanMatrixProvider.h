#ifndef VARODAYANMatrixProvider_H
#define VARODAYANMatrixProvider_H

#include "VarodayanSyndrome.h"
#include "aff3ct/IAff3ctMatrixProvider.h"

namespace icov {
class VarodayanMatrixProvider : public IAff3ctMatrixProvider {
public:
  [[deprecated]] VarodayanMatrixProvider(VarodayanSyndromeManager *tools)
      : m_tools(tools) {}
  ~VarodayanMatrixProvider() = default;

  // IMatrixProvider interface
protected:
  void _buildMatrix(int numcode, AffMatrix &matrix) override;

private:
  std::shared_ptr<VarodayanSyndromeManager> m_tools;
};
} // namespace icov
#endif // VARODAYANMatrixProvider_H
