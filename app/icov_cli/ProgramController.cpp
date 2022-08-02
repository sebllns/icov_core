#include "ProgramController.h"
#include <iostream>

#include "BinaryMode.h"
#include "CheckMode.h"
#include "EncodeMode.h"
#include "ExportMode.h"
#include "SymboleMode.h"
#include "helpers.h"

ProgramController::ProgramController()
{
  BinaryMode::AddProgramOptions(m_options_desc);
  SymboleMode::AddProgramOptions(m_options_desc);
  ExportMode::AddProgramOptions(m_options_desc);
  CheckMode::AddProgramOptions(m_options_desc);
  EncodeMode::AddProgramOptions(m_options_desc);
}

void
ProgramController::launchMode(const std::string& mode_id)
{
  if (mode_id == "binary") {
    std::cout << "binary mode started" << std::endl;
    BinaryMode mode(m_options_map);
    mode.run();
    std::cout << "end binary\n";
  } else if (mode_id == "symbole") {
    std::cout << "symbole mode started" << std::endl;
    SymboleMode mode(m_options_map);
    mode.run();
    std::cout << "end symbole\n";
  } else if (mode_id == "export") {
    std::cout << "export mode started" << std::endl;
    ExportMode mode(m_options_map, codeLength(), dataPath());
    mode.run();
    std::cout << "end export\n";
  } else if (mode_id == "check") {
    std::cout << "check mode started" << std::endl;
    CheckMode mode(m_options_map, dataPath());
    mode.run();
    std::cout << "end check\n";
  } else if (mode_id == "encode") {
    std::cout << "encode mode started" << std::endl;
    EncodeMode mode(m_options_map);
    mode.run();
    std::cout << "end encode\n";
  } else {
    ICOV_THROW_INVALID_ARGUMENT("Unknown program mode: " + mode_id);
  }

  std::cout << "end program\n";
}
