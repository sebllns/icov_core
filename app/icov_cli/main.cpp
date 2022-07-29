#include <iostream>
#include "video_layer/video_common.h"
#include <boost/exception/diagnostic_information.hpp>
#include "ProgramController.h"


/****************************************************************************************/
// MAIN
/****************************************************************************************/
int main(int argc,  char* argv[])
{
    srand((unsigned)time(NULL));

    ProgramController pc;

    try
    {
        icov::video::init_video_lib(argc, argv);
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
