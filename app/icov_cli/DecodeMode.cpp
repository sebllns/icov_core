#include "DecodeMode.h"
#include "extractor/BlockExtractor.h"
#include "extractor/InputMedia.h"
#include "helpers.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "omnidirectional_layer/implementation/EquiRectPicture.h"
#include "omnidirectional_layer/implementation/IcovAccessBlockManager.h"
#include "omnidirectional_layer/implementation/ViewportManagerCenterPadded.h"
#include "tools/DebugTools.h"

#include <boost/format.hpp>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/serialization/vector.hpp>

const char *DecodeMode::params::inputFile_id = "decode-input-file";

void DecodeMode::AddProgramOptions(
    boost::program_options::options_description &desc) {
  desc.add_options()(DecodeMode::params::inputFile_id,
                     po::value<std::string>()->default_value("/tmp/video.icov"),
                     "File to decode");
}

DecodeMode::DecodeMode(const po::variables_map &args) {

  m_parameters.inputFile =
      args[DecodeMode::params::inputFile_id].as<std::string>();

  ICOV_INFO << boost::format("%-15s %-s") % "Input File:" %
                   m_parameters.inputFile;
}

void DecodeMode::run() {
  icov::extractor::InputIcov inputFile(m_parameters.inputFile, 1);
  inputFile.readContentParameterSet();
  const auto cps = inputFile.contentParameterSet();
  ICOV_DEBUG << "Icov version: " << cps.icov_version;
  icov::helpers::assert::check(
      cps.is_block_fixed_size,
      "Only fixed size blocks is available in this version");

  std::cout << inputFile.readBitplaneCount(0) << std::endl;

  EquiRectPicture icov_pic(cps.content_width, cps.content_height);
  IcovAccessBlockManager icov_accessblockMgr;
  ViewportManagerCenterPadded icov_viewport(1.2f);
  BlockPrediction icov_blockPrediction;

  int icu_size = cps.block_fixed_size;
  icov_pic.buildConstantSizeBlocks(icu_size, icu_size);
  icov_accessblockMgr.buildAccessBlocks(icov_pic, icov_viewport);
}
