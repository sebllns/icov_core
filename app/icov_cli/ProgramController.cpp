#include "ProgramController.h"
#include <iostream>

#include "BinaryMode.h"
#include "CheckMode.h"
#include "DecodeMode.h"
#include "DemoMode.h"
#include "EncodeMode.h"
#include "ExportMode.h"
#include "SymboleMode.h"
#include "helpers.h"

ProgramController::ProgramController() {
  BinaryMode::AddProgramOptions(m_options_desc);
  SymboleMode::AddProgramOptions(m_options_desc);
  ExportMode::AddProgramOptions(m_options_desc);
  CheckMode::AddProgramOptions(m_options_desc);
  DemoMode::AddProgramOptions(m_options_desc);
  EncodeMode::AddProgramOptions(m_options_desc);
  DecodeMode::AddProgramOptions(m_options_desc);
}

void ProgramController::launchMode(const std::string &mode_id) {
  if (mode_id == "binary") {
    ICOV_LOG(debug) << "binary mode started";
    BinaryMode mode(m_options_map);
    mode.run();
    ICOV_LOG(debug) << "end binary\n";
  } else if (mode_id == "symbole") {
    ICOV_LOG(debug) << "symbole mode started";
    SymboleMode mode(m_options_map);
    mode.run();
    ICOV_LOG(debug) << "end symbole\n";
  } else if (mode_id == "export") {
    ICOV_LOG(debug) << "export mode started";
    ExportMode mode(m_options_map, codeLength(), dataPath());
    mode.run();
    ICOV_LOG(debug) << "end export\n";
  } else if (mode_id == "check") {
    ICOV_LOG(debug) << "check mode started";
    CheckMode mode(m_options_map, dataPath());
    mode.run();
    ICOV_LOG(debug) << "end check\n";
  } else if (mode_id == "demo") {
    ICOV_LOG(debug) << "demo mode started";
    DemoMode mode(m_options_map);
    mode.run();
    ICOV_LOG(debug) << "end demo\n";
  } else if (mode_id == "encode") {
    ICOV_LOG(debug) << "encode mode started";
    EncodeMode mode(m_options_map);
    mode.run();
    ICOV_LOG(debug) << "end encode\n";
  } else if (mode_id == "decode") {
    ICOV_LOG(debug) << "decode mode started";
    DecodeMode mode(m_options_map);
    mode.run();
    ICOV_LOG(debug) << "end decode\n";
  } else {
    ICOV_THROW_INVALID_ARGUMENT("Unknown program mode: " + mode_id);
  }

  ICOV_LOG(debug) << "end program\n";
}
