#include "EncodeMode.h"

#include "app_common/program/icov_program_options.h"
#include "encoder/IcovMediaEncoder.h"

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/serialization/array_wrapper.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <utility>

using namespace icov;
using namespace icov::omnidirectional_layer;
using namespace icov::bitstream;
using namespace icov::encoder;

const char *EncodeMode::params::inputFile_id = "encode-input-file";
const char *EncodeMode::params::outputFile_id = "encode-output-file";
const char *EncodeMode::params::icuSize_id = "encode-icu-size";
const char *EncodeMode::params::qp_id = "encode-qp";
const char *EncodeMode::params::viewportPadding_id = "encode-viewport-padding";

void EncodeMode::AddProgramOptions(
    boost::program_options::options_description &desc) {
  desc.add_options()

      (EncodeMode::params::inputFile_id,
       po::value<std::string>()->default_value("/tmp/video.mp4"),
       "File to encode (video or image, see documentation for supported file "
       "formats)")

          (EncodeMode::params::outputFile_id,
           po::value<std::string>()->default_value(std::string()),
           "Path to output file (if empty will be same path as input, with "
           ".icov extension)")

              (EncodeMode::params::icuSize_id,
               po::value<int>()->default_value(32), "Size of ICUs blocks")

                  (EncodeMode::params::qp_id,
                   po::value<int>()->default_value(42),
                   "Quantization parameter")

                      (EncodeMode::params::viewportPadding_id,
                       po::value<float>()->default_value(1.2),
                       "Radius used in function to check if a block is "
                       "visible");
}

EncodeMode::EncodeMode(const po::variables_map &args) {
  m_parameters.inputFile =
      args[EncodeMode::params::inputFile_id].as<std::string>();

  m_parameters.encodedFile =
      args[EncodeMode::params::outputFile_id].as<std::string>();
  if (m_parameters.encodedFile.empty()) {
    m_parameters.encodedFile = m_parameters.inputFile;
    m_parameters.encodedFile.replace_extension(".icov");
  }

  m_parameters.icuSize = args[EncodeMode::params::icuSize_id].as<int>();
  m_parameters.qp = args[EncodeMode::params::qp_id].as<int>();

  m_parameters.viewportPadding =
      args[EncodeMode::params::viewportPadding_id].as<float>();

  m_parameters.q_valuesPath = ICOV_Q_VALUES_FILE(args);

  m_parameters.ldpc_params = options::parseLdpcParams(args);

  ICOV_INFO << boost::format("%-15s %-s") % "Input File:" %
                   m_parameters.inputFile;
  ICOV_INFO << boost::format("%-15s %-s ") % "Output File:" %
                   m_parameters.encodedFile;
}

void EncodeMode::run() {
  ICOV_DEBUG << "Start encoding";
  bool enable_z_quantization = true;
  std::vector<float> q_values;
  icov::io::read_vector(m_parameters.q_valuesPath.string(), q_values);
  encoder::IcovMediaEncoder encoder(
      encoder::IcovMediaEncoder::params{
          .InputPath = m_parameters.inputFile,
          .OutputPath = m_parameters.encodedFile,
          .OutputType = encoder::IcovMediaEncoder::BINARY,
          .FrameEncoderParams =
              icov::encoder::FrameEncoder::params{
                  .QP = m_parameters.qp,
                  .ChFormat = CHROMA_420,
                  .ICU_Size =
                      cv::Size(m_parameters.icuSize, m_parameters.icuSize),
                  .BSP_Params =
                      icov::ldpc_codec_layer::QaryDistribution::
                          quantization_params{enable_z_quantization, q_values},
                  .CheckDecoding = false,
                  .MaxRateNoCoding = 3}},
      *m_parameters.ldpc_params);

#warning TODO
#pragma message("TODO")
  throw "TODO";
}
