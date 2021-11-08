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


    M& buildMatrix(int index)
    {
        _buildMatrix(getNumcode(index), getMatrix(index));
        return getMatrix(index);
    }

    const M& getMatrix(int index) const
    {
        return m_matrices[index];
    }

    M& getMatrix(int index)
    {
        return m_matrices[index];
    }

    void addNumcode(icov::index_t numcode)
    {
        m_numcodes.push_back(numcode);
        m_matrices.resize(m_numcodes.size());
    }

    int getMatrixCount() const
    {
        return (int)m_matrices.size();
    }

    icov::index_t getNumcode(int index) const
    {
        return m_numcodes[index];
    }

protected:
    std::vector<icov::index_t> m_numcodes;
    std::vector<M> m_matrices;
    virtual void _buildMatrix(int numcode, M &matrix) = 0;
};
}
#endif // IMatrixProvider_H
