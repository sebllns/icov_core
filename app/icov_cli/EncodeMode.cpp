#include "EncodeMode.h"
#include "encoder/FakeEncoder.h"
#include "encoder/FrameEncoder.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "omnidirectional_layer/implementation/EquiRectPicture.h"
#include "omnidirectional_layer/implementation/IcovAccessBlockManager.h"
#include "tools/DebugTools.h"
#include <boost/format.hpp>

const char *EncodeMode::params::inputFile_id = "encode-input-file";
const char *EncodeMode::params::outputFile_id = "encode-output-file";
const char *EncodeMode::params::icuSize_id = "encode-icu-size";

void EncodeMode::AddProgramOptions(
    boost::program_options::options_description &desc) {
  desc.add_options()

      (EncodeMode::params::inputFile_id,
       po::value<std::string>()->default_value("/tmp/video.mp4"),
       "File to encode")

          (EncodeMode::params::outputFile_id,
           po::value<std::string>()->default_value(std::string()),
           "Path to output file")

              (EncodeMode::params::icuSize_id,
               po::value<int>()->default_value(32), "Size of ICUs blocks");
}

EncodeMode::EncodeMode(const po::variables_map &args) {

  m_parameters.inputFile =
      args[EncodeMode::params::inputFile_id].as<std::string>();

  m_parameters.encodedFile =
      args[EncodeMode::params::outputFile_id].as<std::string>();
  if (m_parameters.encodedFile.empty()) {
    m_parameters.encodedFile = m_parameters.inputFile;
    m_parameters.encodedFile.replace_extension(".icov");

    m_parameters.icuSize = args[EncodeMode::params::icuSize_id].as<int>();

    bool enable_z_quantization = true;
    std::vector<float> q_values = {0.125,   0.375,   0.5625,  0.6875,
                                   0.78125, 0.84375, 0.90625, 0.96875};
    BitSymbolProvider::params bsp_params = {enable_z_quantization, q_values};
    icov::encoder::FrameEncoder::params enc_params = {
        42, CHROMA_420, cv::Size(m_parameters.icuSize, m_parameters.icuSize),
        cv::Size(m_parameters.icuSize, m_parameters.icuSize), bsp_params};

    m_encoder = std::make_unique<icov::encoder::FrameEncoder>(0, enc_params);
  }

  //  CoderFactory::create_coder_yuv(args, m_ycoder, m_uvcoder);

  std::cout << boost::format("%-15s %-s") % "Input File:" %
                   m_parameters.inputFile
            << std::endl;
  std::cout << boost::format("%-15s %-s ") % "Output File:" %
                   m_parameters.encodedFile
            << std::endl;
}

void EncodeMode::run() {
  std::unique_ptr<icov::encoder::InputMedia> input;
  std::ofstream output(m_parameters.encodedFile);

  icov::encoder::instantiateInputMedia(m_parameters.inputFile, input);

  EquiRectPicture icov_pic(input->getWidth(), input->getHeight());
  icov_pic.buildConstantSizeBlocks(m_parameters.icuSize, m_parameters.icuSize);

  // TODO INIT ACCESS BLOCKS !!
  IcovAccessBlockManager icov_accessblockMgr;

  BlockPrediction icov_blockPrediction;
  std::cout << "encode\n";
  m_encoder->encode(output, *input, icov_pic, icov_accessblockMgr,
                    icov_blockPrediction);
  output.close();
}
