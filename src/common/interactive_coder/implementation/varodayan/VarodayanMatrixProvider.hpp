#include "VarodayanMatrixProvider.h"
#include <cstdint>

using namespace icov;

const AffMatrix VarodayanMatrixProvider::_buildMatrix(int numcode)
{
    int m = m_tools->get_m(numcode);
    int n = m_tools->get_n();

    AffMatrix H (n, m);

    std::cout << m << ", " << n << std::endl;

    for (int k = 0; k < (int)n; k++)
    {
        for (uint32_t l = m_tools->jc[k]; l <  m_tools->jc[k + 1]; l++)
        {
            H.add_connection(k, m_tools->getIr(numcode, l));
        }
    }

    return  H;
}
