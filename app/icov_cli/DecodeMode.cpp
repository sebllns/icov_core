#include "DecodeMode.h"
#include "Block.h"
#include "BlockPrediction.h"
#include "SinglePassDecoder.h"
#include "helpers.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/highgui.hpp"
#include "tools/DebugTools.h"
#include "video_layer/vtm_opencv_utility.h"

#include <boost/format.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

const char *DecodeMode::params::inputFile_id = "decode-input-file";
const char *DecodeMode::params::icuSize_id = "decode-icu-size";
const char *DecodeMode::params::qp_id = "decode-qp";
const char *DecodeMode::params::viewportPadding_id = "decode-viewport-padding";

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

  m_parameters.coder_params =
      icov::LdpcCoderFactory::parse_coder_parameters(args);
  icov::helpers::io::read_vector(ICOV_Q_VALUES_FILE(args).string(), m_q_values);

  m_parameters.inputFile =
      args[DecodeMode::params::inputFile_id].as<std::string>();

  auto encodedFile = m_parameters.inputFile;
  encodedFile = m_parameters.inputFile;
  encodedFile.replace_extension(".icov");

  m_parameters.icuSize = args[DecodeMode::params::icuSize_id].as<int>();
  m_parameters.qp = args[DecodeMode::params::qp_id].as<int>();

  m_parameters.viewportPadding =
      args[DecodeMode::params::viewportPadding_id].as<float>();

  bool enable_z_quantization = true;
  std::vector<float> q_values;
  icov::helpers::io::read_vector(ICOV_Q_VALUES_FILE(args).string(), q_values);

  ICOV_INFO << boost::format("%-15s %-s") % "Input File:" %
                   m_parameters.inputFile;
  ICOV_INFO << boost::format("%-15s %-s ") % "Output File:" % encodedFile;
}

void DecodeMode::run() {

  decoder::SinglePassDecoder decoder(m_parameters.inputFile);
  decoder.initialize(*m_parameters.coder_params);

  const int block_count = 1;

  ICOV_DEBUG << "init decoding ICUs";

  ICOV_DEBUG << "start decoding ICUs";

  for (int icu = 0; icu < block_count; ++icu) {
    ICOV_DEBUG << "decode block " << icu;

    bool is_access_block = (icu == 0);

    BlockPrediction::predType block_prediction =
        is_access_block ? BlockPrediction::predType::NONE
                        : BlockPrediction::predType::LEFT;

    BlockVector::const_iterator b =
        decoder.icovNavigation().Visualization->Picture->getById(icu + 1);

    decoder.blockProcessor().startProcess();
    decoder.blockProcessor().processBlock(*b, block_prediction);
    decoder.blockProcessor().endProcess();
  }

  const char *windows_title = "decoded";
  cv::namedWindow(windows_title, cv::WINDOW_NORMAL);

  cv::Mat frame_output(128, 128, CV_8UC3, cv::Scalar(255, 0, 255));
  VTM_OPENCV_Utility::toBGRMat(decoder.getOutputBuf(), frame_output);

  cv::imshow(windows_title, frame_output);
  cv::waitKey(0);
}
