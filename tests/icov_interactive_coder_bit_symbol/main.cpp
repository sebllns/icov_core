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
#include "interactive_coder/implementation/imt/IMTMatrixProvider.hpp"

#include "interactive_coder/implementation/imt_164/IMT164Syndrome.hpp"
#include "interactive_coder/implementation/imt_164/IMT164MatrixProvider.hpp"

#include "interactive_coder/implementation/varodayan/VarodayanSyndrome.hpp"
#include "interactive_coder/implementation/varodayan/VarodayanMatrixProvider.hpp"

#include "TestSequenceInput.h"
#include "TestSequenceResult.h"

//#include <boost/filesystem.hpp>

using namespace aff3ct;
using namespace icov;

/****************************************************************************************/
// I - AFF3CT CONFIG & STRUCTS
/****************************************************************************************/
struct params
{
    std::string database = "database_path";
    int   N         =   1000;     // codeword size
    bool enable_nocoding = false;
    bool enable_z_quantization = true;
    bool enable_binarization_check = false;
    bool enable_q_quantization = true;
    int  n_ite     =   100;
    int si_count = 8;
    std::string coder_type = "null";
    std::string output_dir = "./results_bit_symbol/";
};

struct buffers
{
    std::vector<CodedVal> src_symbols;
    std::vector<CodedVal> sign_mask;
    std::vector<std::vector<CodedVal>> src_bits;
    std::vector<std::vector<CodedVal>> si_bits;
    std::vector<CodedVal> side_info;
    std::vector<LLRVal> llr;
    std::vector<CodedVal> dec_bits;
    std::vector<CodedVal> dec_signs;
    std::vector<CodedVal> dec_symbols;
    std::vector<std::pair<float,float>> sim_result;
    std::vector<float> q_values;
};

struct encode_parameters
{
    int start_code;
    int stop_code;
    int step;
};

void init_params(params &p, int argc, char* argv[])
{
    if(argc < 5) throw std::runtime_error("bad arguments");
    p.coder_type = argv[1];
    p.database = argv[2];
    p.N = atoi(argv[3]);
    p.n_ite = atoi(argv[4]);

//    boost::filesystem::create_directories(p.output_dir);
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


    b.dec_bits      = std::vector<CodedVal  >(p.N, 0);
    b.dec_symbols   = std::vector<CodedVal  >(p.N, 0);
    b.dec_signs =  std::vector<CodedVal  >(p.N, 0);

    std::vector<float> pcrossover;
    DebugTools::read_vector("./data/sim_results/pcrossover", pcrossover);

    std::vector<float> rate;
    DebugTools::read_vector("./data/sim_results/rate", rate);

    b.sim_result.resize(rate.size());

    for(int i = 0 ; i < rate.size() ; i++)
    {
        b.sim_result[i] = std::pair<float,float>(pcrossover[i], rate[i]);
    }

    b.q_values = {0.125, 0.375, 0.5625, 0.6875, 0.78125, 0.84375, 0.90625, 0.96875};
}

float estimate_rate(const std::vector<CodedVal> &src, const std::vector<CodedVal> &si, const std::vector<std::pair<float, float>> &sim_result)
{
//    for(int i = 0 ; i < src.size() ; i++)
//    {
//        std::cout << src[i] << ", ";
//    }

//    std::cout << std::endl;

//    for(int i = 0 ; i < si.size() ; i++)
//    {
//        std::cout << si[i] << ", ";
//    }

    std::cout << std::endl;

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

    //std::cout << "p error = " << p << std::endl;

    int i = 0;
    while(i < (int)sim_result.size() && sim_result[i].first < p)
    {
        i++;
    }

    //todo interpolation ?
    r = sim_result[i].second;

    std::cout << "estimated rate=" << r << std::endl;

    return r;
}

int quantize(int value, int step, int min, int max)
{
    return value == 0 ? 0 : std::clamp((int)((std::abs(value)/value) * step * std::ceil((1.0 * std::abs(value)) / step)), min, max);
}

int quantize(float value, const std::vector<float>& v)
{
    int idx = std::upper_bound(v.begin(), v.end(), value) - v.begin();
    idx = std::clamp(idx, 0, (int)v.size()-1);
    int idx_prev = std::clamp(idx-1, 0, (int)v.size()-1);

    if(std::abs(value - v[idx_prev]) <  std::abs(value - v[idx]))
    {
        idx = idx_prev;
    }
    return idx;
}

/****************************************************************************************/
// III - MAIN
/****************************************************************************************/
int main(int argc,  char* argv[])
{

    auto start_main = std::chrono::steady_clock::now();

    srand((unsigned)time(NULL));

    params p;
    init_params(p, argc, argv);

    buffers b;
    init_buffers(p, b); // create and initialize the buffers required by the modules


    /****************************************************************************************/
    // 2 - INIT ENCODER WITH PARAMS
    /****************************************************************************************/

    std::vector<encode_parameters> encode_params;

    std::cout<<"start interactive coder\n";

    //common part
    LLRProvider llrProvider(p.N);
    Aff3ctDecoder ldpca_dec(p.N);
    ISyndromeManager* syndromeMgr = nullptr;
    IMatrixProvider<AffMatrix>* mb = nullptr;


    std::cout<<"type of coder = "<<p.coder_type<<std::endl;

    int start_matrix, end_matrix;
    int n = 0;

    int inverse_matrix_build = false;

    float min_rate = 0.0f;

    if(p.coder_type == "imt")
    {
        IMTSyndromeManager* imt_synd = new IMTSyndromeManager(p.N);
        syndromeMgr = imt_synd;
        mb = new IMTMatrixProvider(imt_synd, p.database);

        min_rate = 1.0 / 8;

        int n_8 = imt_synd->get_n()/8;
        int n_2 = imt_synd->get_n()/2;
        n = imt_synd->get_n();

        start_matrix = 0;
        end_matrix = n_2 - n_8;

        int pow = std::log2f(p.N);

        encode_params.resize(3);
        encode_params[0].start_code = 0;
        encode_params[0].stop_code = n_2 - n_8;
        // encode_params[0].step = std::floor(4*pow/8);
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
    else if(p.coder_type == "imt164")
    {

        IMT164SyndromeManager* imt_synd = new IMT164SyndromeManager(p.N, 2, 64);
        syndromeMgr = imt_synd;
        mb = new IMT164MatrixProvider(imt_synd, p.database);

        int n_64 = imt_synd->get_n()/64;
        int n_2 = imt_synd->get_n()/2;
        n = imt_synd->get_n();

        start_matrix = 0;
        end_matrix = n_2 - n_64;

        int pow = std::log2f(p.N);

        min_rate = 1.0 / 64;

        encode_params.resize(3);
        encode_params[0].start_code = 0;
        encode_params[0].stop_code = n_2 - n_64;
        // encode_params[0].step = std::floor(4*pow/8);
        encode_params[0].step = std::ceil(pow/8) * 4;

        encode_params[1].start_code = encode_params[0].stop_code;
        encode_params[1].stop_code = n - n_64;
        // encode_params[1].step =  std::floor(5*pow/8);
        encode_params[1].step = encode_params[0].step;

        encode_params[2].start_code = encode_params[1].stop_code;
        encode_params[2].stop_code = 3*n_2 - n_64 + 1;
        // encode_params[2].step =  std::floor(9*pow/8);
        encode_params[2].step = 2 * encode_params[1].step;

        inverse_matrix_build = true;
    }
    else if(p.coder_type == "varodayan")
    {
        VarodayanSyndromeManager* varo_synd = new VarodayanSyndromeManager(p.N);
        varo_synd->read_mat(p.database);
        syndromeMgr = varo_synd;
        mb = new VarodayanMatrixProvider(varo_synd);

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
        throw std::runtime_error("ERROR: Coder type could not be recognized: " + p.coder_type);
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

    interactive_coder->buildMatrices(*mb, numcode, end_code, inverse_matrix_build);

    std::cout<<"init coder\n";
    for(int c = start_matrix; c < end_code; c++)
    {
        //std::cout << c <<std::endl;

        //std::cout << interactive_coder->getMatrix(c).get_n_rows() << "x" << interactive_coder->getMatrix(c).get_n_cols() << ": " << interactive_coder->getMatrix(c).get_cols_max_degree() << ", " << interactive_coder->getMatrix(c).get_rows_max_degree()<<std::endl;

        ldpca_dec.init_decoder(c, p.N, p.n_ite, interactive_coder->getMatrix(c));
    }




    std::string input_dir = "/home/sbelleno/_ICOV/sequences_for_fangping_v02/";


    //6000files_Qary_withoutCodingnoCoding
    //6000files_Qary_CodingnoCoding_q3bits
    std::string result_dir = "/home/sbelleno/_ICOV/results/results_6000files_rates/results_6000files/6000files_Qary_withoutCodingnoCoding/Results_6000files_Qary/";

    std::ofstream measure_file(p.output_dir + "./metric.csv");
    measure_file << "file\tsi\tabs diff\trelative diff\n";

    std::ifstream seq_list(input_dir + "/../files.txt");

    //pano_aaaaajndugdzeh_qp_22_0000__icu_015250_bandGroup_0000
    //pano_aaaaajndugdzeh_qp_22_0000__icu_017795_bandGroup_0000
    //pano_aaaaajndugdzeh_qp_22_0000__icu_017798_bandGroup_0000
    //pano_aaaaajndugdzeh_qp_37_0000__icu_021719_bandGroup_0000
    //pano_aaaaajndugdzeh_qp_27_0000__icu_021719_bandGroup_0000
    //pano_aaaaajndugdzeh_qp_22_0000__icu_021115_bandGroup_0000
    std::string sequence_file = "pano_aaaaajndugdzeh_qp_22_0000__icu_021115_bandGroup_0000";


    while (std::getline(seq_list, sequence_file))
    {

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
        b.sign_mask = std::vector<CodedVal>(p.N, 0);

        for(int i = 0 ; i < p.N ; i++)
        {

            b.sign_mask[i] = b.src_symbols[i] == 0 ? 0 : 1;                    //start encoding


            int x_reconstruction = 0;
            b.src_bits[0][i] = (b.src_symbols[i] < 0);

            for(int j = 1 ; j < lx ; j++)
            {
                b.src_bits[lx-j][i] = (std::abs(b.src_symbols[i]) >> (j-1)) & 1U;
                x_reconstruction |= b.src_bits[lx-j][i] << (j-1);
            }

            //sanity check for X binarization
            x_reconstruction *= (1 - 2*b.src_bits[0][i]);
            if(x_reconstruction != b.src_symbols[i]) throw std::runtime_error("ERROR: Binarization failed: " + std::to_string(x_reconstruction) + " != " + std::to_string(b.src_symbols[i]));
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

        int si_count = std::min(tsi.get_side_info_count(), p.si_count);

        //encode for each SI
        for(int si = 0 ; si < si_count ; si++)
        {
            rate_results.push_back(std::vector<std::vector<float>>(lx, std::vector<float>(2, 0.0)));
            std::cout << "*************************************************************************" << std::endl;

            std::cout << "si=" << si+1 << "/" << si_count << std::endl;

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

            if(p.enable_z_quantization)
            {
                z_min = quantize(z_min, 16, -256, 256);
                z_max = quantize(z_max, 16, -256, 256);
            }

            if(p.enable_q_quantization)
            {
                q = b.q_values[quantize(q, b.q_values)];
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

            BitSymbolProvider bsp(z_min, z_max, y_min, y_max, q, lx);

            std::vector<int> predec(p.N, 0);

            for(int i = 0; i < lx; i++)
            {

                int bitplane = (lx - i - 1);

                //check coding/no-coding when option is enabled
                bool code_enabled = true;
                if(p.enable_nocoding && ((z_min == 0 && z_max == 0) || (i < ly && (b.si_bits[i] == b.src_bits[i]))))
                {
                    code_enabled = false;
                    std::cout << "b=" << i <<" rate="<<0<<std::endl;
                }

                if(code_enabled)
                {
                    //estimate probability and rate
                    float estimated_rate = estimate_rate(b.src_bits[i], i < ly ? b.si_bits[i] : zero_vec, b.sim_result);

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

                    //                if(p.enable_nocoding && llrProvider.no_coding())
                    //                {
                    //                    code_enabled = false;
                    //                    std::cout << "b=" << i <<" rate="<<0<<std::endl;
                    //                }
                    //                else
                    {
                        //save LLR (in case of doping)
                        b.llr = llrProvider.llr();

                        //DebugTools::print_array<LLRVal>(b.llr.data(), b.llr.size(), "llr");

                        //reset rate to 0
                        numcode = 0;

                        //reset error count
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

                            numerrors = interactive_coder->encode(numcode, stored_synd, b.src_bits[i], end_code, param.step, p.n_ite, i == 0 ? b.sign_mask : std::vector<CodedVal>());

                            //encoding done (no error)
                            if(numerrors == 0) break;
                        }




                        //get actual rate from numcode
                        rate_results[si][i][0] = estimated_rate;
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
                                if(i == (lx - 1))
                                {
                                    int x_reconstructed = (1-2*b.src_bits[0][k]) * predec[k];
                                    if(x_reconstructed != b.src_symbols[k]) std::cerr<<"ERROR: X could not be reconstructed: " << x_reconstructed << " != " << b.src_symbols[k] <<std::endl;
                                }
                            }
                        }

                        auto end = std::chrono::steady_clock::now();
                        std::chrono::duration<double> elapsed_seconds = end-start;

                        if(numerrors != 0)
                        {
                            std::cerr<<"ERROR: No convergence for bitplane="<< lx-i-1 <<std::endl;
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
                                std::cerr<<"ERROR: Decoding check failed for bitplane "<< i <<std::endl;
                                std::cerr<<"dec_bits - src_bits = ";
                                for(size_t di = 0 ; di < b.dec_bits.size() ; di++)
                                {
                                    std::cerr << b.dec_bits[di] -  b.src_bits[i][di] << " ";
                                }
                                std::cerr<<std::endl;
                            }
                        }
                    }
                }

                for(int l = 0; l < n ; l++)
                {
                    if(i == 0)
                    {
                        b.dec_symbols[l] = 0;
                        b.dec_signs[l] = code_enabled ? b.dec_bits[l] : b.si_bits[0][l];
                    }
                    else
                    {
                        b.dec_symbols[l] += code_enabled ? b.dec_bits[l] << bitplane : b.si_bits[i][l] << bitplane;
                    }

                    if(bitplane == 0)
                    {
                        b.dec_symbols[l] *= (1 - 2 * b.dec_signs[l]);
                        if(b.dec_symbols[l] != b.src_symbols[l]) std::cerr<<"ERROR: X["<<l<<"] could not be reconstructed: " << b.dec_symbols[l] << " != " << b.src_symbols[l] <<std::endl;
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

        std::ofstream result_file(p.output_dir + "./" + sequence_file + ".txt");

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

        if(p.enable_binarization_check)
        {
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
        }
        result_file.close();


        for(int si = 0; si < rate_results.size() ; si++)
        {
            float icov_rate = 0.0, estim_rate = 0.0, fp_rate = 0.0;
            for(int bp = 0; bp < rate_results[si].size() ; bp++)
            {
                estim_rate += rate_results[si][bp][0];
                icov_rate += rate_results[si][bp][1];
                fp_rate += std::max(tsr["R"].get_data(si,frangping_rate.n_cols - 1 - bp), min_rate);
            }

            measure_file << sequence_file << "\t" << si + 1 << "\t" << std::fixed << std::setw( 7 ) << std::setprecision( 6 ) << std::setfill( '0' ) << (icov_rate - fp_rate) << "\t"<< (icov_rate - fp_rate)/fp_rate << std::endl;
        }

        measure_file.flush();

        std::cout << "*************************************************************************" << std::endl;
        std::cout << "END OF SIMULATION\n";
        std::cout << "Database:\t" << p.database<<  std::endl;
        std::cout << "Code size:\t" << p.N<<  std::endl;
        std::cout << "BP iterations:\t" << p.n_ite<<  std::endl;
        std::cout << "*************************************************************************" << std::endl;
    }

    measure_file.close();

    return 0;
}
