#include "EncodeMode.h"
#include "encoder/FrameEncoder.h"
#include "encoder/OutputMedia.h"
#include "omnidirectional_layer/implementation/EquiRectPicture.h"
#include "omnidirectional_layer/implementation/IcovAccessBlockManager.h"
#include "tools/DebugTools.h"
#include <algorithm>
#include <boost/format.hpp>
#include <boost/serialization/array_wrapper.hpp>
#include <cstdint>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/serialization/vector.hpp>
#include <memory>
#include <utility>

#include "video_layer/encoded_stream.h"

const char *EncodeMode::params::inputFile_id = "encode-input-file";
const char *EncodeMode::params::outputFile_id = "encode-output-file";
const char *EncodeMode::params::icuSize_id = "encode-icu-size";

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
  }

  m_parameters.icuSize = args[EncodeMode::params::icuSize_id].as<int>();

  bool enable_z_quantization = true;
  std::vector<float> q_values;
  icov::helpers::io::read_vector(ICOV_Q_VALUES_FILE.string(), q_values);
  BitSymbolProvider::params bsp_params = {enable_z_quantization, q_values};
  icov::encoder::FrameEncoder::params enc_params = {
      42,
      CHROMA_420,
      cv::Size(m_parameters.icuSize, m_parameters.icuSize),
      cv::Size(m_parameters.icuSize, m_parameters.icuSize),
      bsp_params,
      args,
      true,
      false,
      true};

  auto encoder = std::make_unique<icov::encoder::FrameEncoder>(0, enc_params);
  auto output =
      std::make_shared<icov::encoder::OutputIcov>(m_parameters.encodedFile);

  encoder->setIcovFile(output);

  m_encoder = std::move(encoder);
  m_output = std::move(output);

  ICOV_INFO << boost::format("%-15s %-s") % "Input File:" %
                   m_parameters.inputFile;
  ICOV_INFO << boost::format("%-15s %-s ") % "Output File:" %
                   m_parameters.encodedFile;
}

void EncodeMode::run() {
  std::unique_ptr<icov::encoder::InputMedia> input;
  icov::encoder::instantiateInputMedia(m_parameters.inputFile, input);
  // std::ofstream output(m_parameters.encodedFile, std::ios_base::binary);
  EquiRectPicture icov_pic(input->getWidth(), input->getHeight());
  icov_pic.buildConstantSizeBlocks(m_parameters.icuSize, m_parameters.icuSize);
  //  {
  //    boost::archive::binary_oarchive o_archive(output,
  //                                              boost::archive::no_header);
  //    //    bool *array = new bool[1024];
  //    //    o_archive &boost::serialization::make_array<bool>(array, 1024);
  //    //    delete[] array;
  //    //    output.close();

  //    //    std::vector<bool> array(1024);
  //    //    o_archive &array;

  //    //    std::bitset<1024> array;
  //    //    o_archive &array;

  //    Syndrome s(1024);
  //    s.fill(0);
  //    for (int i = 0; i < s.size(); i += 2) {
  //      s[i] = 1;
  //    }

  //    int byte_size = bitToByteCount(s.size());
  //    icov::byte darr[byte_size];
  //    std::fill(darr, darr + byte_size, 0);
  //    write_syndrome(darr, s);

  //    for (int i = 0; i < byte_size; i++) {
  //      std::cout << std::bitset<8>(darr[i]) << " ";
  //    }
  //    std::cout << std::endl;

  //    Syndrome s2(s.size());
  //    read_syndrome(s2, darr);

  //    for (int i = 0; i < s.size(); ++i) {
  //      if (s[i] != s2[i]) {
  //        ICOV_ERROR << "error";
  //        break;
  //      }
  //    }

  //    o_archive &boost::serialization::make_array(darr, byte_size);

  //    std::cout << "test end" << std::endl;
  //    std::cout << std::endl;
  //  }
  // TODO INIT ACCESS BLOCKS !!
  IcovAccessBlockManager icov_accessblockMgr;
  BlockPrediction icov_blockPrediction;
  ICOV_DEBUG << "Start encoding";

  m_encoder->encode(m_output->stream(), *input, icov_pic, icov_accessblockMgr,
                    icov_blockPrediction);
}
