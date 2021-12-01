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


#include <boost/exception/diagnostic_information.hpp>
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
        std::cerr << "Error: program execution has stopped prematurely" << std::endl;
        std::cerr << boost::current_exception_diagnostic_information() << std::endl;
        std::cerr << pc.optionsDesc() << std::endl;
        return 1;
    }
    catch(...)
    {
        std::cerr << "Fatal error" << std::endl;
        std::cerr << boost::current_exception_diagnostic_information() << std::endl;
        return 2;
    }

    return 0;
}
