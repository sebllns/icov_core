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
};


void init_params(params &p, int argc, char* argv[])
{
    if(argc < 3) throw "bad arguments";
    p.database = std::string(argv[1]);
    p.N = atoi(argv[2]);
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

    std::cout<<"start IMT alist generation\n";

    IMTSyndromeManager imt_tools(p.N);
    IMTMatrixBuilder imt_builder(&imt_tools, p.database);

    std::cout << "*************************************************************************" << std::endl;
    /****************************************************************************************/
    // 2 - BUILD MATRICES
    /****************************************************************************************/
    std::cout<<"build matrices\n";

    imt_builder.read_mat_imt_format();

    std::cout << "*************************************************************************" << std::endl;
    std::cout << "END OF GENERATION\n";
    std::cout << "Database:\t" << p.database<<  std::endl;
    std::cout << "Code size:\t" << p.N<<  std::endl;
    std::cout << "*************************************************************************" << std::endl;


    return 0;
}
