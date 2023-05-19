#include "EncodeDemoMode.h"
#include "encoder/DemoEncoder.h"
#include "icov_program/icov_program_options.h"
#include "omnidirectional_layer/implementation/EquiRectPicture.h"
#include "omnidirectional_layer/implementation/IcovAccessBlockManager.h"

#include <boost/format.hpp>

using namespace icov;
using namespace icov::omnidirectional_layer;
using namespace icov::bitstream;
using namespace icov::encoder;

const char *EncodeDemoMode::params::inputFile_id = "demo-input-file";
const char *EncodeDemoMode::params::outputFile_id = "demo-output-file";
const char *EncodeDemoMode::params::icuSize_id = "demo-icu-size";

void EncodeDemoMode::AddProgramOptions(
    boost::program_options::options_description &desc) {
  desc.add_options()

      (EncodeDemoMode::params::inputFile_id,
       po::value<std::string>()->default_value("/tmp/image.jpg"),
       "Image to use for the demo. Must be an image format supported by OpenCV "
       "(https://docs.opencv.org/4.x/db/deb/tutorial_display_image.html)")

          (EncodeDemoMode::params::outputFile_id,
           po::value<std::string>()->default_value(std::string()),
           "Path to output file")

              (EncodeDemoMode::params::icuSize_id,
               po::value<int>()->default_value(32), "Size of ICUs blocks");
}

EncodeDemoMode::EncodeDemoMode(const po::variables_map &args) {

  m_parameters.inputFile =
      args[EncodeDemoMode::params::inputFile_id].as<std::string>();

  m_parameters.encodedFile =
      args[EncodeDemoMode::params::outputFile_id].as<std::string>();
  if (m_parameters.encodedFile.empty()) {
    m_parameters.encodedFile = m_parameters.inputFile;
    m_parameters.encodedFile.replace_extension(".icov");

    m_parameters.icuSize = args[EncodeDemoMode::params::icuSize_id].as<int>();

    bool enable_z_quantization = true;
    std::vector<float> q_values = {0.125,   0.375,   0.5625,  0.6875,
                                   0.78125, 0.84375, 0.90625, 0.96875};
    QaryDistribution::quantization_params bsp_params = {enable_z_quantization,
                                                        q_values};
    icov::encoder::DemoEncoder::params enc_params = {
        42,
        CHROMA_420,
        cv::Size(m_parameters.icuSize, m_parameters.icuSize),
        cv::Size(m_parameters.icuSize, m_parameters.icuSize),
        bsp_params,
        true,
        false,
        true};

    m_encoder = std::make_unique<icov::encoder::DemoEncoder>(
        0, enc_params, *options::parseLdpcParams(args));
  }

  ICOV_INFO << boost::format("%-15s %-s") % "Input File:" %
                   m_parameters.inputFile;
  ICOV_INFO << boost::format("%-15s %-s ") % "Output File:" %
                   m_parameters.encodedFile;
}

void EncodeDemoMode::run() {
  std::unique_ptr<icov::encoder::InputMedia> input;
  icov::encoder::instantiateInputMedia(m_parameters.inputFile, input);
  icov::assert::equal(
      input->getType(), (int)eMediaType::IMAGE,
      "Only image is allowed as input for demo mode !");
  EquiRectPicture icov_pic(input->getWidth(), input->getHeight());
  icov_pic.buildConstantSizeBlocks(m_parameters.icuSize, m_parameters.icuSize);
  IcovAccessBlockManager icov_accessblockMgr;
  BlockPrediction icov_blockPrediction;
  ICOV_DEBUG << "Start encoding";
  m_encoder->encode(*input, icov_pic, icov_blockPrediction);
}
