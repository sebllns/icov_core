#include "EncodeMode.h"
#include "encoder/FakeEncoder.h"
#include "encoder/FrameEncoder.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "omnidirectional_layer/implementation/EquiRectPicture.h"
#include "omnidirectional_layer/implementation/IcovAccessBlockManager.h"
#include "tools/DebugTools.h"
#include <boost/format.hpp>

const std::string EncodeMode::s_inputFile_id = "encode-input-file";
const std::string EncodeMode::s_outputFile_id = "encode-output-file";

void EncodeMode::AddProgramOptions(
    boost::program_options::options_description &desc) {
  desc.add_options()(s_inputFile_id.c_str(),
                     po::value<std::string>()->default_value("/tmp/video.mp4"),
                     "File to encode")(
      s_outputFile_id.c_str(),
      po::value<std::string>()->default_value(std::string()),
      "Path to output file");
}

EncodeMode::EncodeMode(const po::variables_map &args)
    : m_encoder(std::make_unique<icov::encoder::FrameEncoder>(
          0, 42, CHROMA_420, cv::Size(32, 32), cv::Size(32, 32))),
      m_inputFile(args[s_inputFile_id].as<std::string>()),
      m_encodedFile(args[s_outputFile_id].as<std::string>()) {

  if (m_encodedFile.empty()) {
    m_encodedFile = m_inputFile;
    m_encodedFile.replace_extension(".icov");
  }

  // CoderFactory::create_coder_yuv(args, m_ycoder, m_uvcoder);

  std::cout << boost::format("%-15s %-s") % "Input File:" % m_inputFile
            << std::endl;
  std::cout << boost::format("%-15s %-s ") % "Output File:" % m_encodedFile
            << std::endl;
}

void EncodeMode::run() {
  std::unique_ptr<icov::encoder::InputMedia> input;
  std::ofstream output(m_encodedFile);

  icov::encoder::instantiateInputMedia(m_inputFile, input);

  cv::namedWindow("viewport original", cv::WINDOW_NORMAL);
  cv::imshow("viewport original", input->getCurrentFrame());
  cv::waitKey(0);

  EquiRectPicture icov_pic(input->getWidth(), input->getHeight());
  // picture->buildConstantSizeBlocks(block_size, block_size);

  IcovAccessBlockManager icov_accessblockMgr;
  BlockPrediction icov_blockPrediction;
  std::cout << "encode\n";
  m_encoder->encode(output, *input, icov_pic, icov_accessblockMgr,
                    icov_blockPrediction);
  output.close();
}
