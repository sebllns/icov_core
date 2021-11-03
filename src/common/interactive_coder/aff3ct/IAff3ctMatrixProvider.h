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
        const AffMatrix &buildMatrix(int numcode) override
        {
            if(m_hasMatrix.size() < (numcode +1))
            {
                m_hasMatrix.resize(numcode+1);
            }

            if(m_matrices.size() < (numcode +1))
            {
                m_matrices.resize(numcode+1);
            }

            m_hasMatrix[numcode] = true;

            m_matrices[numcode] = _buildMatrix(numcode);
            return getMatrix(numcode);
        }

        const AffMatrix &getMatrix(int numcode) const override
        {
            return m_matrices[std::min(numcode, (int)m_matrices.size()-1)];
        }

        bool hasMatrix(int numCode) const override
        {
            if(numCode < 0 || numCode >= m_hasMatrix.size())
            {
                return false;
            }
            return m_hasMatrix[numCode];
        }

    protected:
        virtual const AffMatrix _buildMatrix(int numcode) = 0;

    private:
        std::vector<bool> m_hasMatrix;
        std::vector<AffMatrix> m_matrices;
    };

}

#endif // IAFF3CTMatrixProvider_H
