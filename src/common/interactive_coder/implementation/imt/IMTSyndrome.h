#ifndef IMTSYNDROME_H
#define IMTSYNDROME_H

#include "interactive_coder/interface/ISyndrome.h"
#include <fstream>
#include <sstream>

#include <aff3ct.hpp>

using namespace aff3ct;

using AffMatrix = tools::Sparse_matrix;


namespace icov {

/**************************************************/
struct IMTTools : public DefaultSyndromeTools
{

public:

    AffMatrix *h12_ord;
    AffMatrix *h14_ord;
    AffMatrix *h18;

    FastArray<bool> syndrome12;
    FastArray<bool> syndrome14;
    FastArray<bool> syndrome18;

     std::vector<CodedVal> source;



    FastMatrix<uint32_t> *c18;

    std::string root_dir;


    IMTTools(int _n):c18(nullptr)
    {
        this->n = _n;
    }

    virtual int get_m(int numcode) const override
    {
        return  get_n()/8 + numcode;
    }

   virtual int get_n() const override
   {
        if(n == 0 && h18 != nullptr)
        {
            return (int)h18->get_n_rows();
        }
        return this->n;
   }

    u_int32_t get_c18(int m) const
    {
        if(m >= get_n()/8)
        {
            return c18->at(get_n()/4 - m - 1,1) - 1;
        }

        return c18->at(m,0) - 1;
    }

    std::string get_alist_file(int m) const
    {
        return root_dir + "/" + std::to_string(get_n()) + "/alist/" + ("H_" + std::to_string(m) + ".alist");
    }


    template<typename T> FastMatrix<T> read_mat_IMT_format(const std::string& filename)
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

    void read_mat_imt_format()
    {
        std::string dirpath(root_dir + "/" + std::to_string(get_n()));

        auto h12_imt = read_mat_IMT_format<bool>(dirpath + "/H.txt");
        auto c14_imt = read_mat_IMT_format<uint32_t>(dirpath + "/C14.txt");
        auto c18_imt = read_mat_IMT_format<uint32_t>(dirpath + "/C18.txt");

        AffMatrix h12_ord, h14, h14_ord, h18;

        order_combine(h12_imt, c14_imt, h12_ord, h14);
        order_combine(h14, c18_imt, h14_ord, h18);

        //h12_ord.print(true);      //OK
        //h14.print(true);          //OK
        //h14_ord.print(true);      //OK
        //h18.print(true);          //OK


        int n = h12_ord.get_n_rows();
        int m_12 = h12_ord.get_n_cols();
        int m_14 = h14.get_n_cols();
        int m_18 = h18.get_n_cols();

        for(int m = m_18 ; m <= m_12 ; m++)
        {
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
    }


private:
    void order_combine(const FastMatrix<bool>& src, const FastMatrix<uint32_t>& cmb_idx, AffMatrix& ord, AffMatrix& cmb)
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


    void order_combine(const AffMatrix& src, const FastMatrix<uint32_t>& cmb_idx, AffMatrix& ord, AffMatrix& cmb)
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
};

/**************************************************/
class IMTAccumulation : public ISyndromeAccumulation<IMTTools>
{
public:
    IMTAccumulation() = default;

    // ISyndromeAccumulation interface
public:
    virtual void compute(const std::vector<CodedVal> &source_x, IMTTools &tool) const override;
};

/**************************************************/
class IMTStoredSyndrome : public ISyndromeEncodage<IMTTools>
{
public:
    IMTStoredSyndrome() = default;


    // ISyndromeEncodage interface
public:
    virtual void compute(Syndrome &o_stored_syndrome, int numcode, const IMTTools &tool, const LLRProvider& llr_provider) const override;
};



/**************************************************/
class IMTDeaccumulation : public ISyndromeDecodage<IMTTools>
{
public:
    IMTDeaccumulation() = default;

    // ISyndromeDeaccumulation interface
public:
    virtual void compute(Syndrome &o_syndrome, const Syndrome &acc_syndrome, int numcode, const IMTTools &tool, LLRProvider& llr_provider) const override;
};


}
#endif // IMTSYNDROME_H
