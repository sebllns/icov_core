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
    std::vector<CodedVal> src_bits;
    std::vector<CodedVal  > dec_bits;
    std::vector<SIVal> llr;
    std::vector<double> time;
    std::vector<double> decoding_time;
    std::vector<double> numerrors;
    std::vector<double> rate;
    std::vector<double> pcrossover;
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

    b.pcrossover = {0.005607170053021, 0.012986862055522, 0.021539631929892, 0.031124460304790, 0.041692690273656, 0.053239040776806, 0.065786706354577, 0.079382600480655, 0.094097243346146, 0.110027864438359, 0.127304805975877, 0.146102403411897, 0.166657010396903, 0.189297705370615, 0.214501744859832, 0.243003853808948, 0.276040891839788, 0.316019346323608, 0.369127748984521, 0.500000000000000};

    b.rate = std::vector<double>(b.pcrossover.size(), 0);
    b.numerrors = std::vector<double>(b.pcrossover.size(), 0);
    b.time = std::vector<double>(b.pcrossover.size(), 0);
    b.decoding_time = std::vector<double>(b.pcrossover.size(), 0);

    b.src_bits = std::vector<CodedVal>(p.N, 0);
    std::generate(b.src_bits.begin(), b.src_bits.end(), [](){ return (CodedVal)(((((double)rand() / (double)RAND_MAX) < 0.5)) ? 1.0 : 0.0); });

    if(b.src_bits.size() != (ulong)p.N)
    {
        b.src_bits.resize(p.N);
    }

    b.dec_bits      = std::vector<int  >(p.N);
}

/****************************************************************************************/
// III - MAIN
/****************************************************************************************/
int main(int argc,  char* argv[])
{
    auto start_main = std::chrono::steady_clock::now();

    srand((unsigned)time(NULL));

    /****************************************************************************************/
    // 1 - INIT
    /****************************************************************************************/

    params p;
    init_params(p, argc, argv);

    buffers b;
    init_buffers(p, b); // create and initialize the buffers required by the modules


    std::cout<<"start interactive coder\n";

    //common part
    LLRProvider llrProvider(p.N);
    Aff3ctDecoder ldpca_dec;
    ISyndromeManager* syndromeMgr = nullptr;
    IMatrixBuilder<AffMatrix>* mb = nullptr;

    std::cout<<"type of coder = "<<p.coder_type<<std::endl;

    int start_matrix, end_matrix, start_encode, end_encode;

    if(p.coder_type == "imt")
    {
        IMTSyndromeManager* imt_synd = new IMTSyndromeManager(p.N);
        syndromeMgr = imt_synd;
        mb = new IMTMatrixBuilder(imt_synd, p.database);
        start_matrix = 0;
        end_matrix = imt_synd->get_n()/2 - imt_synd->get_n()/8;

        start_encode = 0;
        end_encode = 3*imt_synd->get_n()/2 - imt_synd->get_n()/8;
    }
    else if(p.coder_type == "varodayan")
    {
        VarodayanSyndromeManager* varo_synd = new VarodayanSyndromeManager(p.N);
        varo_synd->read_mat(p.database);
        syndromeMgr = varo_synd;
        mb = new VarodayanMatrixBuilder(varo_synd);

        start_matrix = 0;
        end_matrix = varo_synd->numCodes - 1;

        start_encode = 0;
        end_encode = varo_synd->numCodes - 1;
    }

    auto interactive_coder = new InteractiveCoder<AffMatrix>(&ldpca_dec, syndromeMgr, &llrProvider);

    std::cout << "*************************************************************************" << std::endl;
    /****************************************************************************************/
    // 2 - BUILD MATRICES
    /****************************************************************************************/
    std::cout<<"build matrices\n";

    int n = syndromeMgr->get_n();

    uint32_t numcode = start_matrix;
    uint32_t end_code = end_matrix;

    interactive_coder->buildMatrices(*mb, numcode, end_code+1);

    std::cout << "*************************************************************************" << std::endl;
    /****************************************************************************************/
    // 3 - SYNDROME ACCUMULATION
    /****************************************************************************************/
    syndromeMgr->init_accumulation(b.src_bits);

    std::cout << "*************************************************************************" << std::endl;
    /****************************************************************************************/
    // 4 - INTERACTIVE ENCODING
    /****************************************************************************************/

    std::cout<<"encode\n";
    Syndrome stored_synd(syndromeMgr->get_n());

    for (int ite = 0; ite < p.n_ite_crossover ; ite++)
    {
        for(int i = 0; i < b.pcrossover.size(); i++)
        {
            float progress = 100.0 * (ite*b.pcrossover.size()+i) / (b.pcrossover.size()*p.n_ite_crossover);

            std::cout << "    " << progress << "%                 \r" << std::flush ;

            //std::generate(b.src_bits.begin(), b.src_bits.end(), [](){ return (CodedVal)(((((double)rand() / (double)RAND_MAX) < 0.5)) ? 1.0 : 0.0); });

            syndromeMgr->reset_doping();
            llrProvider.initLLR(b.pcrossover[i], b.src_bits);
            //imt_acc.compute(b.src_bits, imt_tools);

            b.llr = llrProvider.llr();

            numcode = start_encode;
            end_code = end_encode;

            auto start = std::chrono::steady_clock::now();

            int numerrors = interactive_coder->encode(numcode, stored_synd, b.src_bits, end_code+1, p.n_ite);

            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed_seconds = end-start;


            b.rate[i]  +=( ((double)syndromeMgr->get_m(numcode)) / ((double)p.n_ite_crossover * n));
            b.time[i] += elapsed_seconds.count()/ p.n_ite_crossover;
            b.numerrors[i] += (1.0*numerrors)/(p.n_ite_crossover * n);


            if(numerrors != 0)
            {
                std::cerr<<"ERROR: No convergence. Pcrossover="<< b.pcrossover[i]<<std::endl;
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
                if(!interactive_coder->testConvergence(b.dec_bits, b.src_bits))
                {
                    throw "fatal: error decoding";
                }

                end = std::chrono::steady_clock::now();
                elapsed_seconds = end-start;
                b.decoding_time[i] += elapsed_seconds.count()/ p.n_ite_crossover;
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


    /****************************************************************************************/
    // 5 - WRITE RESULTS
    /****************************************************************************************/
    std::cout<<"Write results"<<std::endl;

    icov::DebugTools::write_vector("pcrossover", b.pcrossover,  '\n');
    icov::DebugTools::write_vector("rate", b.rate, '\n');
    icov::DebugTools::write_vector("numerrors", b.numerrors, '\n');
    icov::DebugTools::write_vector("encode_time", b.time, '\n');
    icov::DebugTools::write_vector("decode_time", b.decoding_time, '\n');


    return 0;
}
