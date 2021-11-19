#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <chrono>

#include <aff3ct.hpp>
//#include <boost/filesystem.hpp>

#include "tools/DebugTools.h"

#include "interactive_coder/factory/CoderFactory.h"
#include "interactive_coder/aff3ct/Aff3ctDecoder.h"

#include "io/MatlabTxtMatrix.h"
#include "io/PcmData.h"

using namespace aff3ct;
using namespace icov;

/****************************************************************************************/
// I - AFF3CT CONFIG & STRUCTS
/****************************************************************************************/
struct params
{
    std::string database = "database_path";
    int   N         =   1000;     // codeword size
    std::string coder_type = "null";
};


void init_params(params &p, int argc, char* argv[])
{
    if(argc < 5) throw "bad arguments";
    p.coder_type = argv[1];
    p.database = argv[2];
    p.N = atoi(argv[3]);
}

bool check_matrix(const AffMatrix& m1, const icov::FastMatrix<icov::bit> &m2)
{
    bool equal = true;
    if(m1.get_n_rows() != m2.cols() || m1.get_n_cols() != m2.rows())
    {
        std::cout << "WARNING: [" << m1.get_n_rows() << " x "  << m1.get_n_cols() << "] != [" << m2.cols() << " x "  << m2.rows() << "]" << std::endl;
        equal = false;
    }


    for(int i = 0 ; i < (int)m1.get_n_rows() ; i++)
    {
        for(int j = 0 ; j < (int)m1.get_n_cols() ; j++)
        {
            if(m1.at(i,j) != m2.at(j,i))
            {
                std::cout << "WARNING: difference detected at " <<i << ", "  << j  << std::endl;
                equal = false;
            }
        }
    }

    return equal;
}

icov::FastMatrix<icov::bit> read_h12_imt18(const std::string &dir)
{
    icov::FastMatrix<icov::bit> h12;
    MatlabTxtMatrix::read_matrix(dir + "/H.txt", h12);
    return h12;
}

std::vector<icov::FastMatrix<uint16_t>> read_mapping_files_imt18(const std::string &dir)
{
    std::vector<icov::FastMatrix<uint16_t>> res(2);
    MatlabTxtMatrix::read_matrix(dir + "/C14.txt", res[0]);
    MatlabTxtMatrix::read_matrix(dir + "/C18.txt", res[1]);
    return res;
}

icov::FastMatrix<icov::bit> read_h12_imt164(const std::string &dir)
{
    icov::FastMatrix<icov::bit> h12;
    PcmData::read_matrix(dir + "/H.pcm", h12);
    // std::cout << h12.to_string() << std::endl;
    return h12;
}

std::vector<icov::FastMatrix<uint16_t>> read_mapping_files_imt164(const std::string &dir, const std::vector<int> &levels)
{
    std::vector<icov::FastMatrix<uint16_t>> res(levels.size());

    for(int i = 0 ; i < (int)res.size() ; i++)
    {
        PcmData::read_mapping_vector(dir + "/C1" + std::to_string(levels[i]) + ".pcm", res[i]);
    }

    return res;
}

void accumulate(const icov::FastMatrix<icov::bit> &source, icov::FastMatrix<icov::bit> &dst, int row_src1, int row_src2, int row_dst, std::vector<int> &indexes)
{
    std::vector<int> indexes2;

    //std::cout << "row " << row_dst << " = " << row_src1 << " + " << row_src2 << std::endl;

    dst = icov::FastMatrix<icov::bit>(source.rows()-1, source.cols());


    int row1_idx = 0;
    int row2_idx = 0;

    for(int k = 0 ; k < (int)indexes.size() ; k++)
    {
        if(indexes[k] == row_src1)
        {
            row1_idx = k + row_dst;
        }
        else if(indexes[k] == row_src2)
        {
            row2_idx = k + row_dst;
        }
        else
        {
            indexes2.push_back(indexes[k]);
        }
    }



    for(int j = 0; j < (int)dst.cols() ; j++)
    {
        int offset = 0;


        for(int i = 0; i < (int)dst.rows() ; i++)
        {

            if(i == row_dst)
            {
                dst.at(i,j) = source.at(row1_idx, j) ^ source.at(row2_idx, j);
                offset--;
            }
            else
            {

                if(i < row_dst)
                {
                    offset = 0;
                }
                else if((i+offset) == row1_idx || (i+offset) == row2_idx)
                {
      //              std::cout << "REMOVED " << i << std::endl;
                    offset++;
                }

                dst.at(i, j) = source.at(i + offset, j);
            }
        }
    }


//    for(int i = 0; i < (int)dst.rows() ; i++)
//    {
//        std::cout << i+1 << ":";

//        for(int j = 0; j < (int)dst.cols() ; j++)
//        {
//            if( dst.at(i,j))
//            {
//                std::cout  << " " << j+1;
//            }
//        }
//        std::cout << std::endl;
//    }

    indexes = indexes2;
}

/****************************************************************************************/
// II - MAIN
/****************************************************************************************/
int main(int argc,  char* argv[])
{
    srand((unsigned)time(NULL));

    /****************************************************************************************/
    // 1 - INIT
    /****************************************************************************************/

    params p;
    init_params(p, argc, argv);

    std::cout<<"start interactive coder\n";

    auto interactive_coder = CoderFactory::create_coder(argc, argv);

    std::cout<<"type of coder = "<<p.coder_type<<std::endl;

    int n = 0;

    icov::FastMatrix<icov::bit>  h12;
    std::vector<icov::FastMatrix<uint16_t>> mapping_files;

    int inverse_matrix_build = false;


    std::cout << "*************************************************************************" << std::endl;
    /****************************************************************************************/
    // 2 - BUILD MATRICES
    /****************************************************************************************/
    std::cout<<"build matrices\n";

    n = interactive_coder->syndromeMgr()->get_n();

    icov::FastMatrix<icov::bit> h_dec;

    h_dec = h12;


    //GET NUMCODE CHECK
    int numcode = interactive_coder->matrixProvider()->getNumcode(interactive_coder->matrixProvider()->getMatrixCount()-1);

    //std::cout << h_dec.to_string() << std::endl;

    if(!check_matrix(interactive_coder->getMatrix(numcode), h_dec))
    {
        std::cout << "error " << interactive_coder->syndromeMgr()->get_m(numcode) << std::endl;
    }

    for(auto &mapping : mapping_files)
    {
        std::vector<int> indexes(h_dec.rows());
        std::iota(indexes.begin(), indexes.end(), 0);

        for(int i = 0 ; i < (int)mapping.rows() ; i++)
        {
            numcode--;

            icov::FastMatrix<icov::bit> h_m;

            accumulate(h_dec, h_m, mapping.at(i, 0), mapping.at(i, 1), i, indexes);

            if(!check_matrix(interactive_coder->getMatrix(numcode), h_m))
            {
                //std::cout << "error " << syndromeMgr->get_m(numcode) << std::endl;
            }

            h_dec = h_m;
        }
    }

    return 0;
}
