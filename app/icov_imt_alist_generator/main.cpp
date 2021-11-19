#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <chrono>

#include <aff3ct.hpp>

#include "tools/DebugTools.h"
#include "interactive_coder/InteractiveCoder.h"
#include "interactive_coder/implementation/imt/IMTSyndrome.h"
#include "interactive_coder/implementation/imt/IMTMatrixProvider.h"
#include "interactive_coder/aff3ct/Aff3ctDecoder.h"
#include "interactive_coder/LLRProvider.h"

#include "ProgramController.h"

using namespace aff3ct;
using namespace icov;



/****************************************************************************************/
// MAIN
/****************************************************************************************/
int main(int argc,  char* argv[])
{
    srand((unsigned)time(NULL));

    ProgramController pc;

    try
    {
        pc.processCommandLine(argc, argv);
        pc.launchProgram();
    }
    catch(std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << pc.optionsDesc() << std::endl;
        return 1;
    }
    catch(...)
    {
        std::cerr << "Fatal error" << std::endl;
        return 2;
    }


//    /****************************************************************************************/
//    // 1 - INIT
//    /****************************************************************************************/

//    params p;
//    init_params(p, argc, argv);

//    std::cout<<"start IMT alist generation\n";

//    IMTSyndromeManager imt_tools(p.N);
//    IMTMatrixProvider imt_builder(&imt_tools, p.database);

//    std::cout << "*************************************************************************" << std::endl;
//    /****************************************************************************************/
//    // 2 - BUILD MATRICES
//    /****************************************************************************************/
//    std::cout<<"build matrices\n";

//    imt_builder.read_mat_imt_format();

//    std::cout << "*************************************************************************" << std::endl;
//    std::cout << "END OF GENERATION\n";
//    std::cout << "Database:\t" << p.database<<  std::endl;
//    std::cout << "Code size:\t" << p.N<<  std::endl;
//    std::cout << "*************************************************************************" << std::endl;


    return 0;
}
