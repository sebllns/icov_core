#include "IMTMatrixBuilder.h"
using namespace icov;

#include <cstdint>

AffMatrix *IMTMatrixBuilder::buildMatrix(int numcode) const
{
    int m = m_tools->get_m(numcode);
    int n = m_tools->get_n();

    //FIXME FILENAME
    auto filename = m_tools->get_alist_file(m);

    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::stringstream message;
        message << "'filename' couldn't be opened ('filename' = " << filename << ").";
        throw invalid_argument(__FILE__, __LINE__, __func__, message.str());
    }

    AffMatrix *H = new AffMatrix(tools::AList
    ::read(file));


    if(m == n/8)
    {
        m_tools->h18 = H;
        m_tools->syndrome18.resize(m);
        m_tools->c18 = new FastMatrix<uint32_t>(m_tools->read_mat_IMT_format<uint32_t>(m_tools->root_dir / std::to_string(n)/ "C18.txt"));
    }

    else if(m == n/4)
    {
        m_tools->h14_ord = H;
        m_tools->syndrome14.resize(m);
    }

    else if(m == n/2)
    {
        m_tools->h12_ord = H;
        m_tools->syndrome12.resize(m);
    }

    std::cout << "IMT " << numcode  << ", " << m << ", " << n << ", " << H->get_n_cols() << std::endl;

    return  H;
}
