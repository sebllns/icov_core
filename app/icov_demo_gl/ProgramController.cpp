#include "ProgramController.h"

#include "app_common/program/icov_program_options.h"
#include "modes/DecodeMode.h"
#include "modes/EncodeDemoMode.h"
#include "modes/EncodeMode.h"
#include "modes/OrderingDemoMode.h"

#include <iostream>

ProgramController::ProgramController()
    : IcovProgramBase("binary, symbole, export, check, encode-demo, "
                      "ordering-demo, encode, decode") {
  EncodeDemoMode::AddProgramOptions(m_options_desc);
  EncodeMode::AddProgramOptions(m_options_desc);
  DecodeMode::AddProgramOptions(m_options_desc);
  OrderingDemoMode::AddProgramOptions(m_options_desc);
}

void ProgramController::launchMode(const std::string &mode_id) {
  if (mode_id == "encode-demo") {
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
    ICOV_DEBUG << "end ordering-demo\n";
  } else {
    ICOV_THROW_INVALID_ARGUMENT("Unknown program mode: " + mode_id);
  }

  ICOV_DEBUG << "end program\n";
}
