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
    m_results = m_decoderCtrl.analyzer().InfosGatherer;
    m_requests = m_decoderCtrl.readRequests();
}

void DecodeMode::run() {
  ICOV_DEBUG << "Start decoding ICUs";
  decode();
}

void DecodeMode::decode()
{

  while (!m_requests.empty())
  {
      float pov_x = m_requests.front().viewpoint.angleX;
      float pov_y = m_requests.front().viewpoint.angleY;
      int frame = m_requests.front().frameID;

      m_requests.pop();
      std::cout << "POV " << pov_x << ", " << pov_y << std::endl;

      // decode
      m_decoderCtrl.getDecoder()->request(pov_x, pov_y, frame);
  }
}
