#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>

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
    int   enable_nocoding = false;
    int   n_ite     =   100;
    std::string coder_type = "null";
};

struct buffers
{
    std::vector<CodedVal> src_symbols;
    std::vector<std::vector<CodedVal>> src_bits;
    std::vector<std::vector<CodedVal>> si_bits;
    std::vector<CodedVal> side_info;
    std::vector<LLRVal> llr;
    std::vector<CodedVal  > dec_bits;
    std::vector<std::pair<float,float>> sim_result;
};

struct encode_parameters
{
    int start_code;
    int stop_code;
    int step;
};

void init_params(params &p, int argc, char* argv[])
{
    if(argc < 5) throw "bad arguments";
    p.coder_type = argv[1];
    p.database = argv[2];
    p.N = atoi(argv[3]);
    p.n_ite = atoi(argv[4]);
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


    std::vector<float> pcrossover;
    DebugTools::read_vector("./data/sim_results/pcrossover", pcrossover);

    std::vector<float> rate;
    DebugTools::read_vector("./data/sim_results/rate", rate);

    b.sim_result.resize(rate.size());

    for(int i = 0 ; i < rate.size() ; i++)
    {
        b.sim_result[i] = std::pair<float,float>(pcrossover[i], rate[i]);
    }
}

float estimate_rate(const std::vector<CodedVal> &src, const std::vector<CodedVal> &si, const std::vector<std::pair<float, float>> &sim_result)
{
    float r = 0.0;
    float p = 0.0;

    int lx = src.size();
    int ly = si.size();

    p = std::abs(lx-ly);

    for(int i = 0 ; i < std::min(lx,ly) ; i++)
    {
        if(src[i] != si[i])
        {
            p++;
        }
    }

    p /= src.size();

    int i = 0;
    while(i < (int)sim_result.size() && sim_result[i].first < p)
    {
        i++;
    }

    //todo interpolation
    r = sim_result[i].second;

    std::cout << "estimated rate=" << r << std::endl;

    return r;
}

/****************************************************************************************/
// III - MAIN
/****************************************************************************************/
int main(int argc,  char* argv[])
{
    //pano_aaaaajndugdzeh_qp_22_0000__icu_015250_bandGroup_0000
    //pano_aaaaajndugdzeh_qp_22_0000__icu_017795_bandGroup_0000
    //pano_aaaaajndugdzeh_qp_22_0000__icu_017798_bandGroup_0000
    //pano_aaaaajndugdzeh_qp_37_0000__icu_021719_bandGroup_0000
    //TEST_pano_aaaaajndugdzeh_qp_27_0000__icu_021719_bandGroup_0000
    std::string sequence_file = "pano_aaaaajndugdzeh_qp_22_0000__icu_017798_bandGroup_0000";

    std::string input_dir = "/home/sbelleno/_ICOV/sequences_for_fangping_v02/";
    std::string result_dir = "/home/sbelleno/_ICOV/results/results_6000files_rates/results_6000files/6000files_Qary_withoutCodingnoCoding/Results_6000files_Qary/";

    TestSequenceInput tsi(1024);
    tsi.read_from_file(input_dir + sequence_file + ".txt");

    TestSequenceResult tsr;
    tsr.read_from_file(result_dir + sequence_file + ".mat");

    result_data rs;
    rs.name = "Lsign";
    rs.type = result_data_type::SCALAR;
    rs.n_cols = 1;
    rs.n_rows = 1;
    rs.data.push_back(10);

    bool eq = (rs == tsr["Lsign"]);

    auto start_main = std::chrono::steady_clock::now();

    srand((unsigned)time(NULL));

    params p;
    init_params(p, argc, argv);

    buffers b;
    init_buffers(p, b); // create and initialize the buffers required by the modules

    /****************************************************************************************/
    // 1 - INIT SOURCE X
    /****************************************************************************************/

    b.src_symbols = tsi.get_source();

    //find x_min and x_max values in source X
    int x_min = *std::min_element(b.src_symbols.begin(), b.src_symbols.end());
    int x_max = *std::max_element(b.src_symbols.begin(), b.src_symbols.end());

    //get length for binary representation of X
    int lx =  std::floor(std::log2f(std::max(std::abs(x_min), std::abs(x_max)))) + 2;

    //init bit planes of X
    b.src_bits = std::vector<std::vector<CodedVal>>(lx, std::vector<CodedVal>(p.N, 0));
    for(int i = 0 ; i < p.N ; i++)
    {

        b.src_bits[0][i] = (b.src_symbols[i] < 0);

        std::cout << b.src_symbols[i] << ": " << b.src_bits[0][i];

        for(int j = 1 ; j < lx ; j++)
        {
            b.src_bits[lx-j][i] = (std::abs(b.src_symbols[i]) >> (j-1)) & 1U;
        }
    }

    /****************************************************************************************/
    // 2 - INIT ENCODER WITH PARAMS
    /****************************************************************************************/

    std::vector<encode_parameters> encode_params;

    std::cout<<"start interactive coder\n";

    //common part
    LLRProvider llrProvider(p.N);
    Aff3ctDecoder ldpca_dec(p.N);
    ISyndromeManager* syndromeMgr = nullptr;
    IMatrixBuilder<AffMatrix>* mb = nullptr;


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
    // 3 - BUILD MATRICES
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

    //stored syndrome max size n
    Syndrome stored_synd(syndromeMgr->get_n());

    std::vector<CodedVal> zero_vec(p.N, 0);

    std::vector<std::vector<std::vector<float>>> rate_results;

    int si_count = std::min(tsi.get_side_info_count(), 8);

    //encode for each SI
    for(int si = 0 ; si < si_count ; si++)
    {
        rate_results.push_back(std::vector<std::vector<float>>(lx, std::vector<float>(2, 0.0)));
        std::cout << "*************************************************************************" << std::endl;

        std::cout << "si=" << si+1 << "/" << tsi.get_side_info_count() << std::endl;

        b.side_info = tsi.get_side_info(si);

        //init intrinsic SI parameters
        float q = 0.0;
        int z_min = 999999;
        int z_max = -999999;


        for(int i = 0 ; i < p.N ; i++)
        {
            int z = b.src_symbols[i] - b.side_info[i];

            //q = N0/N
            if(z == 0)
            {
                q += 1.0/p.N;
            }

            //z_min = min(Z[k], 0<=k<N)
            if(z < z_min) z_min = z;

            //z_max = max(Z[k], 0<=k<N)
            if(z > z_max) z_max = z;
        }

        //y_min = min(Y[k], 0<=k<N)
        int y_min = *std::min_element(b.side_info.begin(), b.side_info.end());

        //y_max = max(Y[k], 0<=k<N)
        int y_max = *std::max_element(b.side_info.begin(), b.side_info.end());

        //get length for binary representation of Y
        int ly =  std::floor(std::log2f(std::max(std::abs(y_min), std::abs(y_max)))) + 2;
        ly = lx;

        //init bit planes of Y
        b.si_bits = std::vector<std::vector<CodedVal>>(ly, std::vector<CodedVal>(p.N, 0));
        for(int i = 0 ; i < p.N ; i++)
        {
            b.si_bits[0][i] = (b.side_info[i] < 0);

            for(int j = 1 ; j < ly ; j++)
            {
                b.si_bits[ly-j][i] = (std::abs(b.side_info[i]) >> (j-1)) & 1U;
            }
        }

        BitSymbolProvider bsp(z_min, z_max, y_min, y_max, q);

        std::vector<int> predec(p.N, 0);

        for(int i = 0; i < lx; i++)
        {

            float estimated_rate = estimate_rate(b.src_bits[i], i < ly ? b.si_bits[i] : zero_vec, b.sim_result);
            rate_results[si][i][0] = estimated_rate;

            int bitplane = (lx - i - 1);

            //check coding/no-coding when option is enabled
            bool code_enabled = true;
            if(p.enable_nocoding && ((z_min == 0 && z_max == 0) || (i < ly && (b.si_bits[i] == b.src_bits[i]))))
            {
                code_enabled = false;
                std::cout << "no coding" << std::endl;
            }

            if(code_enabled)
            {
                //std::cout <<  b.pcrossover[i]<<std::endl;
                syndromeMgr->reset_doping();

                if(i == 0)
                {
                    //get LLR for first bitplane level i.e. sign
                    llrProvider.initLLR_s(bsp, b.side_info, b.src_bits[i]);
                }
                else
                {
                    //get LLR for bitplane
                    llrProvider.initLLR_b(i-1, bsp, b.side_info,  b.src_bits[0], predec, b.src_bits[i]);
                }

                //save LLR (in case of doping)
                b.llr = llrProvider.llr();

                //reset rate to 0
                numcode = 0;

                //reset error couny
                int numerrors = -1;

                //syndrome accumulation
                syndromeMgr->init_accumulation(b.src_bits[i]);

                //start encoding
                auto start = std::chrono::steady_clock::now();

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

                    //encoding done (no error)
                    if(numerrors == 0) break;
                }

                //get actual rate from numcode
                double rate = ( ((double)syndromeMgr->get_m(numcode)) / ((double)n));
                rate_results[si][i][1] = rate;


                //double vtruth = tsr["R"].get_data(si, i);

                std::cout << "b=" << i <<" rate="<<rate<<std::endl;

                if(i > 0)
                {
                    //fill predecoded bit values
                    for(int k = 0 ; k < p.N ; k++)
                    {
                        predec[k] = std::abs(b.src_symbols[k]) >> bitplane;
                        // std::cout << predec[k] <<"(" << b.src_symbols[k] <<  ") ";
                    }
                }

                auto end = std::chrono::steady_clock::now();
                std::chrono::duration<double> elapsed_seconds = end-start;

                if(numerrors != 0)
                {
                    std::cerr<<"ERROR: No convergence. bitplane="<< lx-i-1 <<std::endl;
                }
                else
                {
                    //reset doping state and LLR
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
        }
        auto end_main = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_seconds = end_main-start_main;

        std::cout << "Duration:\t" << elapsed_seconds.count()/60 << " mins"<<  std::endl;
        std::cout << "q:\t" << bsp.q0()<<  std::endl;
        std::cout << "z_min:\t" << bsp.z_min()<<  std::endl;
        std::cout << "z_max:\t" << bsp.z_max()<<  std::endl;
        std::cout << "n_bitplane:\t" << lx<<  std::endl;

    }

    std::ofstream result_file("./results_bit_symbol/" + sequence_file + ".txt");


    int sid = 0;

    for(int i = 0; i < 2; i++)
    {
        result_file << ((i == 0) ? "# estimated rate" : "# actual rate") << std::endl;
        for(auto &si : rate_results)
        {
            for(auto &bp : si)
            {
                result_file<< std::fixed << std::setw( 7 ) << std::setprecision( 6 )
                           << std::setfill( '0' ) << bp[i] << " ";
            }
            result_file << std::endl;
        }
        result_file << std::endl;
    }

    result_file <<"# Fangping's rate"  << std::endl;
    auto &frangping_rate = tsr["R"];
    for(int i = 0; i < std::min(si_count, frangping_rate.n_rows); i++)
    {
        for(int j = 0; j < frangping_rate.n_cols ; j++)
        {
            result_file<< std::fixed << std::setw( 7 ) << std::setprecision( 6 )
                       << std::setfill( '0' ) << frangping_rate.get_data(i,frangping_rate.n_cols - 1 - j) << " ";
        }
         result_file << std::endl;
    }

    result_file << std::endl;

    result_file << "# X binarization" << std::endl;

    for(int i = 0 ; i < p.N ; i++)
    {
        result_file << b.src_symbols[i] << "\t   ";
        for(int j = 0 ; j < lx ; j++)
        {
            result_file << b.src_bits[j][i] << " ";
        }
        result_file << std::endl;
    }

    result_file.close();


    std::cout << "*************************************************************************" << std::endl;
    std::cout << "END OF SIMULATION\n";
    std::cout << "Database:\t" << p.database<<  std::endl;
    std::cout << "Code size:\t" << p.N<<  std::endl;
    std::cout << "BP iterations:\t" << p.n_ite<<  std::endl;
    std::cout << "*************************************************************************" << std::endl;

    return 0;
}
