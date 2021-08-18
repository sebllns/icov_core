#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <chrono>

#include <aff3ct.hpp>

#include "tools/DebugTools.h"
#include "interactive_coder/InteractiveCoder.hpp"
#include "interactive_coder/implementation/imt/IMTSyndrome.hpp"
#include "interactive_coder/implementation/imt/IMTMatrixBuilder.hpp"
#include "interactive_coder/aff3ct/Aff3ctDecoder.hpp"
#include "interactive_coder/LLRProvider.h"

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
};

struct buffers
{
    std::vector<CodedVal> src_bits;
    std::vector<CodedVal  > dec_bits;
    std::vector<double> time;
    std::vector<double> numerrors;
    std::vector<double> rate;
    std::vector<double> pcrossover;
};

void init_params(params &p, int argc, char* argv[])
{
    if(argc < 5) throw "bad arguments";
    p.database = std::string(argv[1]);
    p.N = atoi(argv[2]);
    p.n_ite = atoi(argv[3]);
    p.n_ite_crossover = atoi(argv[4]);
}

void init_buffers(const params &p, buffers &b);

void init_buffers(const params &p, buffers &b)
{

    b.pcrossover = {0.005607170053021, 0.012986862055522, 0.021539631929892, 0.031124460304790, 0.041692690273656, 0.053239040776806, 0.065786706354577, 0.079382600480655, 0.094097243346146, 0.110027864438359, 0.127304805975877, 0.146102403411897, 0.166657010396903, 0.189297705370615, 0.214501744859832, 0.243003853808948, 0.276040891839788, 0.316019346323608, 0.369127748984521, 0.500000000000000};

    b.rate = std::vector<double>(b.pcrossover.size(), 0);
    b.numerrors = std::vector<double>(b.pcrossover.size(), 0);
    b.time = std::vector<double>(b.pcrossover.size(), 0);


  b.src_bits      = {0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0};

  if(b.src_bits.size() != (ulong)p.N)
  {
      b.src_bits.resize(p.N);
  }

  b.dec_bits      = std::vector<int  >(p.N);
}

template<typename T> void printVector(std::vector<T> v)
{
    std::cout << "[";
    for(int i = 0; i < v.size() ; i++)
    {
        std::cout << v[i] << ", ";
    }

    std::cout << v[v.size()-1] << "]" << std::endl;
}

/****************************************************************************************/
// III - MAIN
/****************************************************************************************/
int main(int argc,  char* argv[])
{
    srand((unsigned)time(NULL));

    /****************************************************************************************/
    // 1 - INIT
    /****************************************************************************************/

    params p;
    init_params(p, argc, argv);

    buffers b;
    init_buffers(p, b); // create and initialize the buffers required by the modules


    std::cout<<"start interactive IMT coder\n";


    std::cout<<"init coder\n";

    IMTTools imt_tools(p.N);
    LLRProvider llrProvider(p.N);
    Aff3ctDecoder ldpca_dec;
    auto interactive_coder = new InteractiveCoder<AffMatrix, IMTTools>(&ldpca_dec, &imt_tools, &llrProvider);

    std::cout << "*************************************************************************" << std::endl;
    /****************************************************************************************/
    // 2 - BUILD MATRICES
    /****************************************************************************************/
    std::cout<<"build matrices\n";

    imt_tools.root_dir = p.database;
    int n = imt_tools.get_n();

    uint32_t numcode = 0;
    uint32_t end_code = n/2 - n/8;


    IMTMatrixBuilder mb(&imt_tools);
    interactive_coder->buildMatrices(mb, numcode, end_code+1);

    std::cout << "*************************************************************************" << std::endl;
    /****************************************************************************************/
    // 3 - SYNDROME ACCUMULATION
    /****************************************************************************************/
    std::cout<<"compute acc\n";
    IMTAccumulation imt_acc;
    imt_acc.compute(b.src_bits, imt_tools);

    //OK std::cout<< "s12 " << imt_tools.syndrome12.to_string() << std::endl;
    //OK std::cout<< "s14 " << imt_tools.syndrome14.to_string() << std::endl;
    //OK std::cout<< "s18 " << imt_tools.syndrome18.to_string() << std::endl;

    std::cout << "*************************************************************************" << std::endl;
    /****************************************************************************************/
    // 4 - INTERACTIVE ENCODING
    /****************************************************************************************/

    std::cout<<"encode\n";
    Syndrome stored_synd(imt_tools.get_n());

    IMTStoredSyndrome imt_stored;
    IMTDeaccumulation imt_deacc;



    for (int ite = 0; ite < p.n_ite_crossover ; ite++)
    {
        for(int i = 0; i < b.pcrossover.size(); i++)
        {
            float progress = 100.0 * (ite*b.pcrossover.size()+i) / (b.pcrossover.size()*p.n_ite_crossover);

            std::cout << "    " << progress << "%                 \r" << std::flush ;

            imt_tools.source = b.src_bits;
            imt_tools.reset_doping();
            llrProvider.initLLR(b.pcrossover[i], b.src_bits);


            numcode = 0;
            end_code = 2*n - n/8;

            auto start = std::chrono::steady_clock::now();

            int numerrors = interactive_coder->encode(numcode, stored_synd, b.src_bits, end_code+1, p.n_ite, imt_stored, imt_deacc);

            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed_seconds = end-start;


            if(numerrors != 0)
            {
                std::cout<<"ERROR: No convergence. Pcrossover="<< b.pcrossover[i]<<std::endl;
            }


            b.rate[i]  +=( ((double)imt_tools.get_m(numcode)) / ((double)p.n_ite_crossover * n));
            b.time[i] += elapsed_seconds.count()/ p.n_ite_crossover;
            b.numerrors[i] += (1.0*numerrors)/(p.n_ite_crossover * n);

            //std::cout << "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b";
        }
    }

    std::cout << "100%" <<  std::endl;

    std::cout << "*************************************************************************" << std::endl;
    std::cout << "END OF SIMULATION\n";
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
    icov::DebugTools::write_vector("nummerrors", b.numerrors, '\n');
    icov::DebugTools::write_vector("time", b.time, '\n');


    return 0;
}
