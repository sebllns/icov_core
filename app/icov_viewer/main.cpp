#include "ProgramController.h"
#include <boost/exception/diagnostic_information.hpp>
#include <iostream>
/****************************************************************************************/
// MAIN
/****************************************************************************************/
int main(int argc, char *argv[]) {
  ProgramController pc;
  return icov_main(argc, argv, pc);
}
