#include "DecodeMode.h"
#include "app_common/decoder/SinglePassDecoder.h"
#include "app_common/program/icov_program_options.h"

#include <boost/format.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <vector>

DecodeMode::DecodeMode(const path::path_t &icov_file, const po::variables_map &args) : m_decoderCtrl(icov_file, args)
{
    ICOV_DEBUG << "Parse program arguments";

    m_decoderCtrl.initAnalyzer();
    m_requests = m_decoderCtrl.readRequests();
}

void DecodeMode::run() {
  ICOV_DEBUG << "Start decoding ICUs";
  decode();
}

void DecodeMode::decode() { m_decoderCtrl.decodePositions(m_requests); }
