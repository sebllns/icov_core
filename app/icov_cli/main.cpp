#include "ProgramController.h"
#include "video_layer/video_common.h"
#include <boost/exception/diagnostic_information.hpp>
#include <iostream>
/****************************************************************************************/
// MAIN
/****************************************************************************************/
int
main(int argc, char* argv[])
{
  srand((unsigned)time(NULL));

  try {
    icov::video::init_video_lib(argc, argv);
  } catch (...) {
    std::cerr << "Fatal error" << std::endl;
    std::cerr << boost::current_exception_diagnostic_information() << std::endl;
    return 2;
  }

  ProgramController pc;
  icov_main(argc, argv, pc);

  return 0;
}
