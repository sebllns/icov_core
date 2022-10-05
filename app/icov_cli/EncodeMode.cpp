#include "EncodeMode.h"
#include "encoder/FakeEncoder.h"
#include "encoder/FrameEncoder.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "omnidirectional_layer/implementation/EquiRectPicture.h"
#include "omnidirectional_layer/implementation/IcovAccessBlockManager.h"
#include "tools/DebugTools.h"
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

const std::string EncodeMode::s_inputFile_id = "encode-input-file";

void EncodeMode::AddProgramOptions(
    boost::program_options::options_description &desc) {
  desc.add_options()(s_inputFile_id.c_str(),
                     po::value<std::string>()->default_value("/tmp/video.mp4"),
                     "File to encode");
}

EncodeMode::EncodeMode(const po::variables_map &args)
    : m_encoder(std::make_unique<FrameEncoder>(1)),
      m_inputFile(args[s_inputFile_id].as<std::string>()) {
  // CoderFactory::create_coder_yuv(args, m_ycoder, m_uvcoder);
  // m_encoder->openFile(m_inputFile);
}

void EncodeMode::run() {
  EquiRectPicture icov_pic(8888, 4444);
  // picture->buildConstantSizeBlocks(block_size, block_size);

  IcovAccessBlockManager icov_accessblockMgr;
  std::ofstream encoded_file(fs::path(m_inputFile).replace_extension(".icov"));
  std::cout << "encode\n";
  // m_encoder->encode(encoded_file, icov_pic, icov_accessblockMgr);
  encoded_file.close();
}
