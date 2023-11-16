#include "ProgramController.h"
#include "opencv2/videoio.hpp"
#include "video_layer/video_common.h"
#include <boost/exception/diagnostic_information.hpp>
#include <iostream>
/****************************************************************************************/
// MAIN
/****************************************************************************************/
int main(int argc, char *argv[]) {

  cv::VideoCapture video(
      "/home/sbelleno/Videos/360_vr_master_series_ _free_download_ "
      "_london_park_ducks_swans_resized_1k.mp4");

  cv::VideoCapture video2(
      "/home/sbelleno/Videos/360_vr_master_series_ _free_download_ "
      "_london_park_ducks_swans_resized_1k.mp4");

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
