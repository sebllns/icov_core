#include "DecodeMode.h"
#include "decoder/SinglePassDecoder.h"
#include "encoder/IcovMediaEncoder.h"
#include "helpers.h"
#include "icov_program/icov_program_options.h"
#include "omnidirectional_layer/factory.h"
#include "omnidirectional_layer/geometry.h"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/highgui.hpp"
#include "tools/DebugTools.h"
#include "video_layer/ExtractedStreamDebugger.h"
#include "video_layer/vtm_opencv_utility.h"

#include <boost/format.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

const char *DecodeMode::params::InputFile_ID = "decode-input-file";
const char *DecodeMode::params::IcuSize_ID = "decode-icu-size";
const char *DecodeMode::params::QP_ID = "decode-qp";
const char *DecodeMode::params::ViewportPadding_ID = "decode-viewport-padding";

void DecodeMode::AddProgramOptions(
    boost::program_options::options_description &desc) {
  desc.add_options()

      (DecodeMode::params::InputFile_ID,
       po::value<std::string>()->default_value("/tmp/video.mp4"),
       "File to encode")

          (DecodeMode::params::IcuSize_ID, po::value<int>()->default_value(32),
           "Size of ICUs blocks")

              (DecodeMode::params::QP_ID, po::value<int>()->default_value(42),
               "Quantization parameter")

                  (DecodeMode::params::ViewportPadding_ID,
                   po::value<float>()->default_value(1.2),
                   "Radius used in function to check if a block is "
                   "visible");
}

DecodeMode::DecodeMode(const po::variables_map &args) {
  ICOV_DEBUG << "Parse program arguments";

  m_parameters.input_file =
      args[DecodeMode::params::InputFile_ID].as<std::string>();

  m_parameters.ldpc_params = icov::options::parseLdpcParams(args);

  m_parameters.bsp_params.enable_z_quantization = true;

  icov::helpers::io::read_vector(ICOV_Q_VALUES_FILE(args).string(),
                                 m_parameters.bsp_params.q_values);

  m_parameters.icu_size = args[DecodeMode::params::IcuSize_ID].as<int>();

  m_parameters.qp = args[DecodeMode::params::QP_ID].as<int>();

  m_parameters.viewport_padding =
      args[DecodeMode::params::ViewportPadding_ID].as<float>();

  ICOV_INFO << boost::format("%-15s %-s") % "Input File:" %
                   m_parameters.input_file;
}

void DecodeMode::run() {
  ICOV_DEBUG << "Start encoding";
  encode();

  // ICOV_DEBUG << "start decoding ICUs";
  //  decode();

  //  check();

  //  display();
}

void DecodeMode::encode() {
  bitstream::VideoEncodingStreamCache::get_mutable_instance().clear();
  bitstream::VideoStreamDebugger::get_mutable_instance().reset();
  bitstream::VideoEncodingStreamCache::get_mutable_instance().setFrameCount(
      100);

  m_encoder = std::make_unique<encoder::IcovMediaEncoder>(
      encoder::IcovMediaEncoder::params{
          .InputPath = m_parameters.input_file,
          .OutputPath = m_parameters.input_file.replace_extension(".icov"),
          .OutputType = encoder::IcovMediaEncoder::TEXT,
          .FrameEncoderParams =
              icov::encoder::FrameEncoder::params{
                  .QP = m_parameters.qp,
                  .ChFormat = CHROMA_420,
                  .ICU_Size =
                      cv::Size(m_parameters.icu_size, m_parameters.icu_size),
                  .BSP_Params = m_parameters.bsp_params,
                  .CheckDecoding = true,
                  .MaxRateNoCoding = 3}},
      *m_parameters.ldpc_params);

  const float fov_x_degree = 60;
  const point2d resolution(1920, 1080);
  angle_radian fov = factory::fov(fov_x_degree, factory::y_x_ratio(resolution));
  viewport_geometry vpg = factory::default_geometry(fov, resolution);
  m_encoder->defaultEncode(m_parameters.viewport_padding, vpg);
}

void DecodeMode::decode() {
  // INIT DECODER
  m_decoder = std::make_unique<decoder::SinglePassDecoder>(
      m_encoder->output_media().getPath());
  m_decoder->initialize(*m_parameters.ldpc_params);

  // CHECK CPS
  icov::helpers::assert::equal(
      m_encoder->output_media().contentParameterSet(),
      m_decoder->icovFile().contentParameterSet(),
      "ContentParameterSet not equal between Encoder & Decoder");

  // DECODE FULL FRAME
  m_decoder->decodeFullFrame();
}

void DecodeMode::check() {}

void DecodeMode::display() {
  const char *windows_title = "decoded";
  cv::namedWindow(windows_title, cv::WINDOW_NORMAL);

  cv::Mat frame_output(128, 128, CV_8UC3, cv::Scalar(255, 0, 255));
  VTM_OPENCV_Utility::toBGRMat(m_decoder->getOutputBuf(), frame_output);

  cv::imshow(windows_title, frame_output);
  cv::waitKey(0);
}
