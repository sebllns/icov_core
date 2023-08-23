#include "DecodeMode.h"
#include "app_common/decoder/SinglePassDecoder.h"
#include "app_common/program/icov_program_options.h"

#include <boost/format.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

DecodeMode::DecodeMode(const po::variables_map &args)
{
    ICOV_DEBUG << "Parse program arguments";

    m_parameters.input_file = options::parseInputFile(args);

    m_parameters.ldpc_params = icov::options::parseLdpcParams(args);

    ICOV_INFO << boost::format("%-15s %-s") % "Input File:" % m_parameters.input_file;
}

void DecodeMode::run() {
  ICOV_DEBUG << "Start decoding ICUs";
  decode();
}

void DecodeMode::decode()
{
  // INIT DECODER
  m_decoder = std::make_unique<decoder::SinglePassDecoder>();
  m_decoder->initLocal(decoder::SinglePassDecoder::params{.DummyViewport = false},

                       m_parameters.input_file,

                       *m_parameters.ldpc_params);

  // DECODE FULL FRAME
  m_decoder->start();
}
