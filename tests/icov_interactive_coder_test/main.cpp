#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <chrono>

#include <aff3ct.hpp>

#include "debug/DebugTools.hpp"
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
    std::string filename = "./data/imt/";
    int   K         =   4096;     // number of information bits
    int   N         =   K;     // codeword size
    int   n_ite	=   100;
    int n_ite_crossover = 100;

    //IMT R = M/N M varying from 12 to N
    float R;                   // code rate (R=K/N)
};
void init_params(params &p);


struct modules
{
    std::vector<bool> pct_pattern;
};
void init_modules(const params &p, modules &m);

struct buffers
{
    std::vector<int  > src_bits;
    std::vector<uint32_t > acc_bits;
    std::vector<float> LLRs;
    std::vector<int  > dec_bits;
    std::vector<double> time;
    std::vector<double> numerrors;
    std::vector<double> rate;
    std::vector<double> pcrossover;
};
void init_buffers(const params &p, buffers &b);


/****************************************************************************************/
// INITIALIZATION FUNCTIONS
/****************************************************************************************/

void init_params(params &p)
{
    p.R = (float)p.K / (float)p.N;
    std::cout << "# * Simulation parameters: "              << std::endl;
    std::cout << "#    ** Info. bits (K) = " << p.K         << std::endl;
    std::cout << "#    ** Frame size (N) = " << p.N         << std::endl;
    std::cout << "#    ** Code rate  (R) = " << p.R         << std::endl;
    std::cout << "#"                                        << std::endl;
}

void init_modules(const params &p, modules &m)
{
};



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

  b.LLRs  = std::vector<float >(p.N);
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
int main(int argc, char** argv)
{
    srand((unsigned)time(NULL));

    icov::FastMatrix<double> mat(3,4);

    mat.at(0,2) = 1;
    mat.at(1,1) = 123;
    mat.at(2,3) = 13;

    std::cout << mat.to_string() << std::endl;

    /****************************************************************************************/
    // TESTS STRUCTURES PERFORMANCES (TODO : move into dedicated program)
    /****************************************************************************************/

//    int nv = 10000;

//    auto start = std::chrono::steady_clock::now();

//    icov::BitVector vec(nv);


//    for(int k = 0; k < 1000; k++)
//    {
//        vec.set(0, 1);
//        vec.set(500, 1);
//        vec.set(1000, 1);
//        for(int i = 1; i < nv; i++)
//        {
//            vec.set(i, vec[i]^vec[i-1]);
//        }
//    }
//    auto end = std::chrono::steady_clock::now();

//    std::chrono::duration<double> elapsed_seconds = end-start;
//    std::cout<<"bitset="<<elapsed_seconds.count()/1000<<std::endl;

//    getchar();



//    start = std::chrono::steady_clock::now();
//    std::vector<bool> vec2(nv);

//    for(int k = 0; k < 1000; k++)
//    {
//        vec2[0] = 1;
//        vec2[500] = 1;
//        vec2[1000] = 1;
//        for(int i = 1; i < nv; i++)
//        {
//            vec2[i] = vec2[i]^vec2[i-1];
//        }
//    }
//    end = std::chrono::steady_clock::now();

//    elapsed_seconds = end-start;
//    std::cout<<"vector="<<elapsed_seconds.count()/1000<<std::endl;

//    getchar();



//    start = std::chrono::steady_clock::now();
//    icov::FastArray<icov::bit> vec3(nv);

//    for(int k = 0; k < 1000; k++)
//    {
//        vec3[0] = 1;
//        vec3[500] = 1;
//        vec3[1000] = 1;
//        for(int i = 1; i < nv; i++)
//        {
//            vec3[i] ^= vec2[i-1];
//        }
//    }
//    end = std::chrono::steady_clock::now();

//    elapsed_seconds = end-start;
//    std::cout<<"FastArray="<<elapsed_seconds.count()/1000<<std::endl;

//    getchar();

    /****************************************************************************************/
    // 1 - INIT
    /****************************************************************************************/

    params p;  init_params (p   ); // create and initialize the parameters defined by the user
    modules m; init_modules(p, m); // create and initialize the modules
    buffers b; init_buffers(p, b); // create and initialize the buffers required by the modules


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

    imt_tools.root_dir = p.filename;
    imt_tools.read_mat_imt_format();

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
        std::cout << "*************************************************************************" << std::endl;
        std::cout << std::endl << ite + 1 << "/"  << p.n_ite_crossover <<  std::endl;
        for(int i = 0; i < b.pcrossover.size(); i++)
        {
            std::cout << "pcrossover=" << b.pcrossover[i] << std::endl;

            imt_tools.source = b.src_bits;
            llrProvider.initLLR(b.pcrossover[i], b.src_bits);


            numcode = 0;
            end_code = 2*n - n/8;

            auto start = std::chrono::steady_clock::now();

            int numerrors = interactive_coder->encode(numcode, stored_synd, b.src_bits, b.LLRs, end_code+1, p.n_ite, imt_stored, imt_deacc);

            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed_seconds = end-start;
            std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";


            if(numerrors == 0)
            {
                std::cout<<"convergence has been reached for m="<<imt_tools.get_m(numcode)<<std::endl;
            }
            else
            {
                std::cout<<"No convergence. Error count e="<<numerrors<<std::endl;
            }

           // std::cout<<"synd="<<stored_synd.to_string()<<std::endl;

            b.rate[i]  +=( ((double)imt_tools.get_m(numcode)) / ((double)p.n_ite_crossover * n));
            b.time[i] += elapsed_seconds.count()/ p.n_ite_crossover;
            b.numerrors[i] += (1.0*numerrors)/(p.n_ite_crossover * n);
        }
    }


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
