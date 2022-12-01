#ifndef IMatrixProvider_H
#define IMatrixProvider_H

#include "common.h"

namespace icov {

template <class M> class IMatrixProvider {
public:
  IMatrixProvider() = default;
  virtual ~IMatrixProvider() = default;

  M &buildMatrix(int index) {
    ICOV_LOG(trace) << __func__ << "(" << index << ") -> " << getNumcode(index);
    index = std::min(index, (int)m_matrices.size() - 1);
    _buildMatrix(getNumcode(index), getMatrix(index));
    return getMatrix(index);
  }

  const M &getMatrix(int index) const {
    return m_matrices[std::min(index, (int)m_matrices.size() - 1)];
  }

  M &getMatrix(int index) {
    return m_matrices[std::min(index, (int)m_matrices.size() - 1)];
  }

  void addNumcode(icov::index_t numcode, bool doping) {
    m_numcodes.push_back(numcode);
    if (!doping) {
      m_matrices.resize(m_numcodes.size());
      // std::cout << numcode << "->"  << m_matrices.size() << std::endl;
    }
  }

  int getMatrixCount() const { return (int)m_matrices.size(); }

  int getNumCodesCount() const { return (int)m_numcodes.size(); }

  icov::index_t getNumcode(int index) const { return m_numcodes[index]; }

  bool is_doping(int index) const { return index >= getMatrixCount(); }

protected:
  std::vector<icov::index_t> m_numcodes;
  std::vector<M> m_matrices;
  virtual void _buildMatrix(int numcode, M &matrix) = 0;
};
} // namespace icov
#endif // IMatrixProvider_H
