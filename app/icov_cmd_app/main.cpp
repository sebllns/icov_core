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

    return 0;
}
