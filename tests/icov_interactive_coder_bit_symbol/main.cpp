#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <chrono>

#include <aff3ct.hpp>
//#include <boost/filesystem.hpp>

#include "tools/DebugTools.h"
#include "interactive_coder/InteractiveCoder.hpp"
#include "interactive_coder/aff3ct/Aff3ctDecoder.hpp"
#include "interactive_coder/LLRProvider.h"

#include "interactive_coder/implementation/imt/IMTSyndrome.hpp"
#include "interactive_coder/implementation/imt/IMTMatrixBuilder.hpp"

#include "interactive_coder/implementation/varodayan/VarodayanSyndrome.hpp"
#include "interactive_coder/implementation/varodayan/VarodayanMatrixBuilder.hpp"

#include "TestSequenceInput.h"
#include "TestSequenceResult.h"

using namespace aff3ct;
using namespace icov;

/****************************************************************************************/
// I - AFF3CT CONFIG & STRUCTS
/****************************************************************************************/
struct params
{
    std::string database = "database_path";
    int   N         =   1000;     // codeword size
    int   n_ite	=   100;
    int n_ite_crossover = 10;
    std::string coder_type = "null";
};

struct buffers
{
    std::vector<CodedVal> src_symbols;
    std::vector<std::vector<CodedVal>> src_bits;
    std::vector<CodedVal> side_info;
    std::vector<LLRVal> llr;
    std::vector<CodedVal  > dec_bits;
};

struct encode_parameters
{
    int start_code;
    int stop_code;
    int step;
};

void init_params(params &p, int argc, char* argv[])
{
    if(argc < 6) throw "bad arguments";
    p.coder_type = argv[1];
    p.database = argv[2];
    p.N = atoi(argv[3]);
    p.n_ite = atoi(argv[4]);
    p.n_ite_crossover = atoi(argv[5]);
}

void init_buffers(const params &p, buffers &b);

void init_buffers(const params &p, buffers &b)
{
    int x_min = -128;
    int x_max = 127;

    int y_min = -64;
    int y_max = 63;

    b.src_symbols = std::vector<CodedVal>(p.N, 0);
    std::generate(b.src_symbols.begin(), b.src_symbols.end(), [x_min, x_max](){ return (CodedVal)(rand()%(x_max-x_min+1) + x_min); });

    b.side_info = std::vector<CodedVal>(p.N, 0);
    std::generate(b.side_info.begin(), b.side_info.end(), [y_min, y_max](){ return (CodedVal)(rand()%(y_max-y_min+1) + y_min); });


    b.dec_bits      = std::vector<int  >(p.N);
}

/****************************************************************************************/
// III - MAIN
/****************************************************************************************/
int main(int argc,  char* argv[])
{
    TestSequenceInput tsi(1024);
    tsi.read_from_file("/home/sbelleno/_ICOV/sequences_for_fangping_v02/pano_aaaaajndugdzeh_qp_22_0000__icu_018612_bandGroup_0000.txt");


    TestSequenceResult tsr;
    tsr.read_from_file("/home/sbelleno/_ICOV/results/results_6000files_rates/results_6000files/6000files_Qary_withoutCodingnoCoding/Results_6000files_Qary/pano_aaaaajndugdzeh_qp_27_0000__icu_019748_bandGroup_0000.mat");

    result_data rs;
    rs.name = "Lsign";
    rs.type = result_data_type::SCALAR;
    rs.n_cols = 1;
    rs.n_rows = 1;
    rs.data.push_back(10);

    bool eq = (rs == tsr["Lsign"]);


    auto start_main = std::chrono::steady_clock::now();

    srand((unsigned)time(NULL));

    /****************************************************************************************/
    // 1 - INIT
    /****************************************************************************************/

    params p;
    init_params(p, argc, argv);

    buffers b;
    init_buffers(p, b); // create and initialize the buffers required by the modules

    std::vector<encode_parameters> encode_params;

    std::cout<<"start interactive coder\n";

    //common part
    LLRProvider llrProvider(p.N);
    Aff3ctDecoder ldpca_dec(p.N);
    ISyndromeManager* syndromeMgr = nullptr;
    IMatrixBuilder<AffMatrix>* mb = nullptr;


    float q = 0.5;
    int x_min = *std::min_element(b.src_symbols.begin(), b.src_symbols.end());
    int x_max = *std::max_element(b.src_symbols.begin(), b.src_symbols.end());

    int y_min = *std::min_element(b.side_info.begin(), b.side_info.end());
    int y_max = *std::max_element(b.side_info.begin(), b.side_info.end());

    int z_min = x_min-y_max;
    int z_max = x_max-y_min;

    BitSymbolProvider bsp(z_min, z_max, y_min, y_max, q);

    int lx = bsp.x_binary_length()+1;
    b.src_bits = std::vector<std::vector<CodedVal>>(lx, std::vector<CodedVal>(p.N, 0));

    for(int i = 0 ; i < p.N ; i++)
    {
        b.src_bits[0][i] = (b.src_symbols[i] < 0);

        for(int j = 1 ; j < lx ; j++)
        {
            b.src_bits[j][i] = (b.src_symbols[i] >> (j-1)) & 1U;
        }
    }

    std::cout<<"type of coder = "<<p.coder_type<<std::endl;

    int start_matrix, end_matrix;
    int n = 0;

    if(p.coder_type == "imt")
    {
        IMTSyndromeManager* imt_synd = new IMTSyndromeManager(p.N);
        syndromeMgr = imt_synd;
        mb = new IMTMatrixBuilder(imt_synd, p.database);

        int n_8 = imt_synd->get_n()/8;
        int n_2 = imt_synd->get_n()/2;
        n = imt_synd->get_n();

        start_matrix = 0;
        end_matrix = n_2 - n_8;

        int pow = std::log2f(p.N);

        encode_params.resize(3);
        encode_params[0].start_code = 0;
        encode_params[0].stop_code = n_2 - n_8;
        //encode_params[0].step = std::floor(4*pow/8);
        encode_params[0].step = std::ceil(pow/8) * 4;

        encode_params[1].start_code = encode_params[0].stop_code;
        encode_params[1].stop_code = n - n_8;
        // encode_params[1].step =  std::floor(5*pow/8);
        encode_params[1].step = encode_params[0].step;

        encode_params[2].start_code = encode_params[1].stop_code;
        encode_params[2].stop_code = 3*n_2 - n_8 + 1;
        // encode_params[2].step =  std::floor(9*pow/8);
        encode_params[2].step = 2 * encode_params[1].step;
    }
    else if(p.coder_type == "varodayan")
    {
        VarodayanSyndromeManager* varo_synd = new VarodayanSyndromeManager(p.N);
        varo_synd->read_mat(p.database);
        syndromeMgr = varo_synd;
        mb = new VarodayanMatrixBuilder(varo_synd);

        start_matrix = 0;
        end_matrix = varo_synd->numCodes - 1;

        encode_params.resize(1);
        encode_params[0].start_code = 0;
        encode_params[0].stop_code = varo_synd->numCodes;
        encode_params[0].step = 1;
    }
    else
    {
        //error
        return 1;
    }

    auto interactive_coder = new InteractiveCoder<AffMatrix>(&ldpca_dec, syndromeMgr, &llrProvider);

    std::cout << "*************************************************************************" << std::endl;
    /****************************************************************************************/
    // 2 - BUILD MATRICES
    /****************************************************************************************/
    std::cout<<"build matrices\n";

    n = syndromeMgr->get_n();

    uint32_t numcode = start_matrix;
    uint32_t end_code = end_matrix + 1;

    interactive_coder->buildMatrices(*mb, numcode, end_code);

    std::cout<<"init coder\n";
    for(int c = start_matrix; c < end_code; c++)
    {
        std::cout << c <<std::endl;
        ldpca_dec.init_decoder(c, p.N, p.n_ite, interactive_coder->getMatrix(c));
    }


    std::cout << "*************************************************************************" << std::endl;
    /****************************************************************************************/
    // 4 - INTERACTIVE ENCODING
    /****************************************************************************************/

    std::cout<<"encode\n";
    Syndrome stored_synd(syndromeMgr->get_n());

    std::vector<int> predec(p.N, 0);

    for(int i = 0; i < lx; i++)
    {
        int bitplane = (lx - i - 1);
        /****************************************************************************************/
        // SYNDROME ACCUMULATION
        /****************************************************************************************/
        syndromeMgr->init_accumulation(b.src_bits[i]);

        numcode = 0;

        float progress = 100.0 * i / lx;

        //std::cout << "    " << progress << "%                 \r" << std::flush ;

        //std::cout <<  b.pcrossover[i]<<std::endl;
        syndromeMgr->reset_doping();

        if(i == 0)
        {
            llrProvider.initLLR_s(bsp, b.side_info, b.src_bits[i]);
        }
        else
        {
            llrProvider.initLLR_b(i-1, bsp, b.side_info,  b.src_bits[0], predec, b.src_bits[i]);
        }

        b.llr = llrProvider.llr();

        auto start = std::chrono::steady_clock::now();

        int numerrors = -1;

        if(numcode  >= (uint32_t) encode_params.back().stop_code)
        {
            numcode =  encode_params.back().start_code;
        }

        for(auto &param: encode_params)
        {
            numcode = std::max(numcode, (uint32_t) param.start_code);
            end_code = param.stop_code;

            if(end_code <= numcode)
            {
                continue;
            }

            //std::cout << "test code in [" << numcode << ", " << end_code << "[" << std::endl;

            numerrors = interactive_coder->encode(numcode, stored_synd, b.src_bits[i], end_code, param.step, p.n_ite);

            if(numerrors == 0) break;
        }

        for(int k = 0 ; k < p.N ; k++)
        {
            predec[k] = std::abs(b.src_symbols[k]) >> bitplane;
            std::cout << predec[k] <<"(" << b.src_symbols[k] <<  ") ";
        }


         std::cout<<"code="<<numcode<<std::endl;

        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_seconds = end-start;




        if(numerrors != 0)
        {
            std::cerr<<"ERROR: No convergence. bitplane="<< lx-i-1 <<std::endl;
        }
        else
        {
            syndromeMgr->reset_doping();

            for(int l = 0; l < n ; l++)
            {
                llrProvider[l] = b.llr[l];
            }

            start = std::chrono::steady_clock::now();
            interactive_coder->decode(b.dec_bits, numcode, p.n_ite, stored_synd);
            end = std::chrono::steady_clock::now();
            elapsed_seconds = end-start;
            if(!interactive_coder->testConvergence(b.dec_bits, b.src_bits[i]))
            {
                std::cerr<<"err no decoding "<< lx-i <<std::endl;
                //throw "fatal: error decoding";
            }
        }
    }


    std::cout << "    100%" <<  std::endl;

    auto end_main = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end_main-start_main;

    std::cout << "*************************************************************************" << std::endl;
    std::cout << "END OF SIMULATION\n";
    std::cout << "Duration:\t" << elapsed_seconds.count()/60 << " mins"<<  std::endl;
    std::cout << "Database:\t" << p.database<<  std::endl;
    std::cout << "Code size:\t" << p.N<<  std::endl;
    std::cout << "BP iterations:\t" << p.n_ite<<  std::endl;
    std::cout << "Simulation iterations:\t" << p.n_ite_crossover<<  std::endl;
    std::cout << "*************************************************************************" << std::endl;

    return 0;
}
