#include "ProgramController.h"

#include "InteractiveDecoder.h"
#include "app_common/decoder/SinglePassDecoder.h"
#include "app_common/encoder/IcovEncoder.h"
#include "app_common/program/icov_program_options.h"
#include "icov_path.h"

#include <boost/filesystem/path.hpp>
#include <iostream>
#include <memory>

ProgramController::ProgramController() : IcovProgramBase("simulation, icov") {}

void ProgramController::launchMode(const std::string &mode_id)
{

    // mode == simulation
    // TODO

    // mode == icov

    auto ldpc_params = parseLdpcParams(args());

    encoder::IcovEncoder::params enc_params(args());
    encoder::IcovEncoder encoder(enc_params, *ldpc_params);
    encoder.run();

    InteractiveDecoder::params dec_params(args());
    dec_params.icov_file = enc_params.m_outputfile;
    InteractiveDecoder decoder(dec_params, *ldpc_params);
    decoder.run();

    ICOV_DEBUG << "end program\n";
}
