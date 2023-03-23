#include "DecodeMode.h"
#include "Block.h"
#include "BlockPrediction.h"
#include "CodingStructure.h"
#include "Common.h"
#include "SinglePassDecoder.h"
#include "common_types.h"
#include "decoder/ChannelDecoder.h"
#include "encoded_stream.h"
#include "encoder/BinaryOutputIcov.h"
#include "encoder/FrameEncoder.h"
#include "encoder/InputMedia.h"
#include "extractor/InputMedia.h"
#include "helpers.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "omnidirectional_layer/factory.h"
#include "omnidirectional_layer/implementation/EquiRectPicture.h"
#include "omnidirectional_layer/implementation/IcovAccessBlockManager.h"
#include "omnidirectional_layer/implementation/ViewportManagerCenterPadded.h"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/highgui.hpp"
#include "tools/DebugTools.h"
#include "typedef.h"
#include "video_layer/Encodingstructure.h"
#include "video_layer/ICOV_BufGroups.h"
#include "video_layer/ICOV_IntraPrediction.h"
#include "video_layer/ICOV_Quant.h"
#include "video_layer/ICOV_Transform2d.h"
#include "video_layer/ICOV_Units.h"
#include "video_layer/vtm_opencv_utility.h"

#include <algorithm>
#include <boost/format.hpp>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/serialization/vector.hpp>
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
  BitSymbolProvider::params bsp_params = {enable_z_quantization, q_values};
  icov::encoder::FrameEncoder::params enc_params;
  //  = {
  //      m_parameters.qp,
  //      CHROMA_420,
  //      cv::Size(m_parameters.icuSize, m_parameters.icuSize),
  //      cv::Size(m_parameters.icuSize, m_parameters.icuSize),
  //      bsp_params,
  //      args,
  //      true,
  //      false,
  //      true,
  //      3};

  std::unique_ptr<icov::encoder::FrameEncoder>
      encoder; //= std::make_unique<icov::encoder::FrameEncoder>(enc_params);
  auto output = std::make_shared<icov::encoder::BinaryOutputIcov>(encodedFile);

  output->contentParameterSet().icov_version =
      "icov_decode_check" ICOV_CORE_VERSION;
  output->contentParameterSet().content_framecount = 1;
  output->contentParameterSet().is_block_fixed_size = true;
  output->contentParameterSet().block_fixed_size = static_cast<icov::byte>(32);
  output->contentParameterSet().content_chroma_format =
      (icov::byte)enc_params.ChFormat;
  encoder->setIcovFile(output);

  m_encoder = std::move(encoder);
  m_output = std::move(output);

  ICOV_INFO << boost::format("%-15s %-s") % "Input File:" %
                   m_parameters.inputFile;
  ICOV_INFO << boost::format("%-15s %-s ") % "Output File:" % encodedFile;
}

void DecodeMode::run() {

  std::unique_ptr<icov::encoder::InputMedia> input;
  icov::encoder::instantiateInputMedia(m_parameters.inputFile, input);
  EquiRectPicture icov_pic(input->getWidth(), input->getHeight());
  IcovAccessBlockManager icov_accessblockMgr;
  ViewportManagerCenterPadded icov_viewport(m_parameters.viewportPadding);
  BlockPrediction icov_blockPrediction;

  decoder::SinglePassDecoder decoder(m_parameters.inputFile);
  decoder.initialize(*m_parameters.coder_params);

  //  const auto cps = inputFile.contentParameterSet();
  //  ICOV_DEBUG << "Icov version: " << cps.icov_version;
  //  ICOV_DEBUG << (int)cps.content_chroma_format;
  //  ICOV_DEBUG << (int)cps.content_height;
  //  ICOV_DEBUG << (int)cps.content_width;
  //  ICOV_DEBUG << (int)cps.block_fixed_size;
  //  ICOV_DEBUG << (int)cps.qp;

  //  const int maxLog2TrDynamicRange = 15;
  //  const Int internalBitDepth[MAX_NUM_CHANNEL_TYPE] = {8, 8};
  //  const ChromaFormat chFrmt = (ChromaFormat)cps.content_chroma_format;
  //  const int qp = (int)cps.qp;
  //  const int icu_size = (int)cps.block_fixed_size;
  //  const int n = icu_size * icu_size;
  //  const int channelCount = 3;
  const int block_count = 1;

  //  const int sourceWidth = (int)cps.content_width;
  //  const int sourceHeight = (int)cps.content_height;
  //  const int margin_x_360luma = 2;

  //  const UnitArea extendedFrameArea(
  //      chFrmt,
  //      Area(0, 0, sourceWidth + 2 * margin_x_360luma, sourceHeight)); //???
  //  const UnitArea mainFrameArea(
  //      chFrmt, Area(margin_x_360luma, 0, sourceWidth, sourceHeight));

  //  auto m_vvc_bufs = std::make_shared<icov::video::BufGroups>();
  //  m_vvc_bufs->create(PIC_ORIGINAL, extendedFrameArea);
  //  m_vvc_bufs->create(PIC_RECONSTRUCTION, extendedFrameArea);
  //  m_vvc_bufs->create(PIC_PREDICTION, extendedFrameArea);

  //  const PelUnitBuf predictionUnitBuf = m_vvc_bufs->get(PIC_PREDICTION);
  //  const auto reconstructionUnitBuffer =
  //  m_vvc_bufs->getBuf(PIC_RECONSTRUCTION);

  //  std::vector<std::unique_ptr<decoder::ChannelDecoder>> channelCoder;
  //  UnitArea blockArea(chFrmt, Area(0, 0, icu_size, icu_size));
  //  for (int c = 0; c < channelCount; c++) {

  //    const auto compID = static_cast<ComponentID>(c);

  //    const ChannelType channelType = toChannelType(compID);
  //    const int bit_depth = internalBitDepth[channelType];
  //    const icov::video::QpParam cQP(qp, channelType, chFrmt, bit_depth, 0,
  //    0);

  //    auto ldpc_args = m_parameters.coder_params->clone();

  //    ldpc_args->n = blockArea.block(compID).area();

  //    channelCoder.push_back(std::make_unique<decoder::ChannelDecoder>(
  //        decoder::ChannelDecoder::params{cQP, compID, bit_depth,
  //                                        maxLog2TrDynamicRange},
  //        *ldpc_args, m_vvc_bufs));
  //  }

  //  std::vector<int> bitplaneCount(channelCount);
  //  std::vector<NumcodeIdxVec> syndSizes(channelCount);

  //  icov::video::TransformUnit dec_tu(blockArea, true, true, true);

  //  PelStorage dec_block;
  //  dec_block.create(blockArea);

  //  inputFile.initBuffer(block_count, n);
  //  inputFile.readBlockPos();

  //  icov::video::Quant quant;
  //  TCoeff uiAbsSum;
  //  icov::video::Transform2D transform2D;
  //  icov::video::IntraPrediction intraPred;

  //  icov::factory::Visualization360 visualization360(
  //      {.width = decoder.icovFile().contentParameterSet().content_width,
  //       .height = decoder.icovFile().contentParameterSet().content_height,
  //       .viewportPadding =
  //           decoder.icovFile().contentParameterSet().viewport_padding});

  //  visualization360.init(
  //      decoder.icovFile().contentParameterSet().block_fixed_size);

  //  EquiRectPicture icov_pic(cps.content_width, cps.content_height);
  //  IcovAccessBlockManager icov_accessblockMgr;
  //  ViewportManagerCenterPadded icov_viewport(1.2f);
  //  BlockPrediction icov_blockPrediction;

  icov::video::EncodingStructure encodingStruct;

  ICOV_DEBUG << "init decoding ICUs";

  //  encodingStruct
  //      .create360VideoInteractiveCodingUnits_withoutAccessBlockComputation(
  //          chFrmt, sourceWidth, sourceHeight, margin_x_360luma, icu_size,
  //          icu_size, icu_size, icu_size);

  // intraPred.init(chFrmt);

  //  icov_pic.buildConstantSizeBlocks(icu_size, icu_size);
  //  icov_accessblockMgr.buildAccessBlocks(icov_pic, icov_viewport);

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

    //    int x_block = b->parent()->col(b->u());
    //    int y_block = b->parent()->row(b->v());

    //    Position pos(static_cast<float>(x_block) /
    //    static_cast<float>(icu_size),
    //                 static_cast<float>(y_block) /
    //                 static_cast<float>(icu_size));

    //    const uint icuIndex = encodingStruct.get_icuIndex(pos);
    //    auto icu_enc = encodingStruct.getICU(icuIndex);
  }

  const char *windows_title = "decoded";
  cv::namedWindow(windows_title, cv::WINDOW_NORMAL);

  cv::Mat frame_output(128, 128, CV_8UC3, cv::Scalar(255, 0, 255));
  VTM_OPENCV_Utility::toBGRMat(decoder.getOutputBuf(), frame_output);

  cv::imshow(windows_title, frame_output);
  cv::waitKey(0);
}
