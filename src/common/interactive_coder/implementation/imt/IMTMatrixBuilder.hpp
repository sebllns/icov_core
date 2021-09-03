#include "IMTMatrixBuilder.h"

#include <cstdint>
#include <fstream>
#include <sstream>

using namespace icov;

AffMatrix *IMTMatrixBuilder::buildMatrix(int numcode) const
{
    int m = m_tools->get_m(numcode);
    int n = m_tools->get_n();

    auto filename = get_alist_file(m);

    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::stringstream message;
        message << "'filename' couldn't be opened ('filename' = " << filename << ").";
        throw aff3ct::tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
    }

    AffMatrix *H = new AffMatrix(tools::AList
    ::read(file));


    if(m == n/8)
    {
        m_tools->h18 = H;
        m_tools->syndrome18.resize(m);
        m_tools->c18 = new FastMatrix<uint32_t>(read_mat_IMT_format<uint32_t>(root_dir + "/" + std::to_string(n) + "/C18.txt"));
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


std::string IMTMatrixBuilder::get_alist_file(int m) const
{
    return root_dir + "/" + std::to_string(m_tools->get_n()) + "/alist/" + ("H_" + std::to_string(m) + ".alist");
}


template<typename T> FastMatrix<T> IMTMatrixBuilder::read_mat_IMT_format(const std::string& filename) const
{
    std::ifstream file(filename);

    if (!file.is_open())
    {
        std::stringstream message;
        message << "'filename' couldn't be opened ('filename' = " <<  filename << ").";
        std::cerr<<message.str()<<std::endl;
        throw message.str();
    }

    uint32_t rows = 0;
    uint32_t cols = 0;

    file >> rows;
    file >> cols;

    FastMatrix<T> mat(rows, cols);
    T val;

    for(int c = 0 ; c < cols ; c++ )
    {
        for(int r = 0 ; r < rows ; r++ )
        {
            file >> val;
            mat.at(r, c) = val;
//                std::cout << r << ", " << c << " = " << val << std::endl;
        }
//            std::cout << std::endl;
    }

    return mat;
}

void IMTMatrixBuilder::read_mat_imt_format()
{
    std::string dirpath(root_dir + "/" + std::to_string(m_tools->get_n()));

    auto h12_imt = read_mat_IMT_format<bool>(dirpath + "/H.txt");
    auto c14_imt = read_mat_IMT_format<uint32_t>(dirpath + "/C14.txt");
    auto c18_imt = read_mat_IMT_format<uint32_t>(dirpath + "/C18.txt");

    AffMatrix h12_ord, h14, h14_ord, h18;

    order_combine(h12_imt, c14_imt, h12_ord, h14);
    order_combine(h14, c18_imt, h14_ord, h18);

    int n = h12_ord.get_n_rows();
    int m_12 = h12_ord.get_n_cols();
    int m_14 = h14.get_n_cols();
    int m_18 = h18.get_n_cols();

    for(int m = m_18 ; m <= m_12 ; m++)
    {
        int progress = 100.0 * (m-m_18) / (m_12-m_18+1);

        std::cout << "    " << progress << "%                          \r" << std::flush ;

        AffMatrix hdec(n, m);

        int ind_erased = m <= m_14 ? m_14 - m : m_12 - m;
        AffMatrix &mat_start = m <= m_14 ? h18 : h14;
        AffMatrix &mat_end = m <= m_14 ? h14_ord : h12_ord;

        //m_18 -> h18
        //m_14 -> h14_ord
        //m_12 -> h12_ord

        for(int j = 0; j < m ; j++)
        {
            for(int i = 0; i < n ; i++)
            {
                AffMatrix &mat = (j < ind_erased) ? mat_start : mat_end;
                if(mat.at(i, j)) hdec.add_connection(i,j);
            }
        }

        std::ofstream alist_file(get_alist_file(m));
        if (!alist_file.is_open())
        {
            std::stringstream message;
            message << "'G_save_path' could not be opened ('alist_file' = \"" << get_alist_file(m) << "\").";
            throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
        }

        tools::AList::write(hdec, alist_file);
    }
    std::cout << "    100%          " <<  std::endl;
}

void IMTMatrixBuilder::order_combine(const FastMatrix<bool>& src, const FastMatrix<uint32_t>& cmb_idx, AffMatrix& ord, AffMatrix& cmb) const
{
    ord = AffMatrix::zero(src.cols(), src.rows());
    cmb = AffMatrix::zero(src.cols(), src.rows()/2);

    for(int i = 0 ; i < cmb_idx.rows() ; i++)
    {
        int row_first = cmb_idx.at(i, 0) - 1;
        int row_last = cmb_idx.at(i, 1) - 1;
        for(int j = 0 ; j < src.cols(); j++)
        {
            bool first = src.at(row_first, j);
            bool last = src.at(row_last, j);

            if(first)
            {
               // std::cout << "first" << std::endl;
                ord.add_connection(j, i);
            }

            if(last)
            {
               // std::cout << "last" << std::endl;
                ord.add_connection(j, src.rows() - 1 - i);
            }

            if(first ^ last)
            {
               // std::cout << "first ^last" << std::endl;
                cmb.add_connection(j,i);
            }
        }
    }
}


void IMTMatrixBuilder::order_combine(const AffMatrix& src, const FastMatrix<uint32_t>& cmb_idx, AffMatrix& ord, AffMatrix& cmb) const
{
    ord = AffMatrix::zero(src.get_n_rows(), src.get_n_cols());
    cmb = AffMatrix::zero(src.get_n_rows(), src.get_n_cols()/2);

    for(int i = 0 ; i < cmb_idx.rows() ; i++)
    {
        int row_first = cmb_idx.at(i, 0) - 1;
        int row_last = cmb_idx.at(i, 1) - 1;
        for(int j = 0 ; j < src.get_n_rows(); j++)
        {
            bool first = src.at(j, row_first);
            bool last = src.at(j, row_last);

            if(first)
            {
                ord.add_connection(j, i);
            }

            if(last)
            {
                ord.add_connection(j, src.get_n_cols() - 1 - i);
            }

            if(first ^ last)
            {
                cmb.add_connection(j,i);
            }
        }
    }
}
