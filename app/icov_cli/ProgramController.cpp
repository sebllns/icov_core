#include "ProgramController.h"
#include <iostream>

#include "BinaryMode.h"
#include "CheckMode.h"
#include "DecodeMode.h"
#include "EncodeDemoMode.h"
#include "EncodeMode.h"
#include "ExportMode.h"
#include "OrderingDemoMode.h"
#include "SymboleMode.h"
#include "helpers.h"

ProgramController::ProgramController() {
  BinaryMode::AddProgramOptions(m_options_desc);
  SymboleMode::AddProgramOptions(m_options_desc);
  ExportMode::AddProgramOptions(m_options_desc);
  CheckMode::AddProgramOptions(m_options_desc);
  EncodeDemoMode::AddProgramOptions(m_options_desc);
  EncodeMode::AddProgramOptions(m_options_desc);
  DecodeMode::AddProgramOptions(m_options_desc);
  OrderingDemoMode::AddProgramOptions(m_options_desc);
}

void ProgramController::launchMode(const std::string &mode_id) {
  if (mode_id == "binary") {
    ICOV_DEBUG << "binary mode started";
    BinaryMode mode(m_options_map);
    mode.run();
    ICOV_DEBUG << "end binary\n";
  } else if (mode_id == "symbole") {
    ICOV_DEBUG << "symbole mode started";
    SymboleMode mode(m_options_map);
    mode.run();
    ICOV_DEBUG << "end symbole\n";
  } else if (mode_id == "export") {
    ICOV_DEBUG << "export mode started";
    ExportMode mode(m_options_map, codeLength(), dataPath());
    mode.run();
    ICOV_DEBUG << "end export\n";
  } else if (mode_id == "check") {
    ICOV_DEBUG << "check mode started";
    CheckMode mode(m_options_map, dataPath());
    mode.run();
    ICOV_DEBUG << "end check\n";
  } else if (mode_id == "encode-demo") {
    ICOV_DEBUG << "encode demo mode started";
    EncodeDemoMode mode(m_options_map);
    mode.run();
    ICOV_DEBUG << "end demo\n";
  } else if (mode_id == "encode") {
    ICOV_DEBUG << "encode mode started";
    EncodeMode mode(m_options_map);
    mode.run();
    ICOV_DEBUG << "end encode\n";
  } else if (mode_id == "decode") {
    ICOV_DEBUG << "decode mode started";
    DecodeMode mode(m_options_map);
    mode.run();
    ICOV_DEBUG << "end decode\n";
  } else if (mode_id == "ordering-demo") {
    ICOV_DEBUG << "ordering demo mode started";
    OrderingDemoMode mode(m_options_map);
    mode.run();
    ICOV_DEBUG << "end decode\n";
  } else {
    ICOV_THROW_INVALID_ARGUMENT("Unknown program mode: " + mode_id);
  }

  ICOV_DEBUG << "end program\n";
}
