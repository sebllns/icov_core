#include "DecodeMode.h"
#include "Block.h"
#include "BlockPrediction.h"
#include "CodingStructure.h"
#include "Common.h"
#include "common_types.h"
#include "decoder/ChannelDecoder.h"
#include "encoded_stream.h"
#include "extractor/BlockExtractor.h"
#include "extractor/InputMedia.h"
#include "helpers.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "omnidirectional_layer/implementation/EquiRectPicture.h"
#include "omnidirectional_layer/implementation/IcovAccessBlockManager.h"
#include "omnidirectional_layer/implementation/ViewportManagerCenterPadded.h"
#include "opencv2/core/mat.hpp"
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

  m_parameters.coder_params = icov::CoderFactory::parse_coder_parameters(args);
  icov::helpers::io::read_vector(ICOV_Q_VALUES_FILE(args).string(), m_q_values);
}

void DecodeMode::run() {
  icov::extractor::InputIcov inputFile(m_parameters.inputFile);
  inputFile.readContentParameterSet();
  const auto cps = inputFile.contentParameterSet();
  ICOV_DEBUG << "Icov version: " << cps.icov_version;
  ICOV_DEBUG << (int)cps.content_chroma_format;
  ICOV_DEBUG << (int)cps.content_height;
  ICOV_DEBUG << (int)cps.content_width;
  ICOV_DEBUG << (int)cps.block_fixed_size;
  ICOV_DEBUG << (int)cps.qp;

  const int maxLog2TrDynamicRange = 15;
  const Int internalBitDepth[MAX_NUM_CHANNEL_TYPE] = {8, 8};
  const ChromaFormat chFrmt = (ChromaFormat)cps.content_chroma_format;
  const int qp = (int)cps.qp;
  const int icu_size = (int)cps.block_fixed_size;
  const int n = icu_size * icu_size;
  const int channelCount = 3;
  const int block_count = 2;

  const int sourceWidth = (int)cps.content_width;
  const int sourceHeight = (int)cps.content_height;
  const int margin_x_360luma = 2;

  const UnitArea extendedFrameArea(
      chFrmt,
      Area(0, 0, sourceWidth + 2 * margin_x_360luma, sourceHeight)); //???
  const UnitArea mainFrameArea(
      chFrmt, Area(margin_x_360luma, 0, sourceWidth, sourceHeight));

  auto m_vvc_bufs = std::make_shared<icov::video::BufGroups>();
  m_vvc_bufs->create(PIC_ORIGINAL, extendedFrameArea);
  m_vvc_bufs->create(PIC_RECONSTRUCTION, extendedFrameArea);
  m_vvc_bufs->create(PIC_PREDICTION, extendedFrameArea);

  //  const PelUnitBuf predictionUnitBuf = m_vvc_bufs->get(PIC_PREDICTION);
  //  const auto reconstructionUnitBuffer =
  //  m_vvc_bufs->getBuf(PIC_RECONSTRUCTION);

  std::vector<std::unique_ptr<decoder::ChannelDecoder>> channelCoder;
  UnitArea blockArea(chFrmt, Area(0, 0, icu_size, icu_size));
  for (int c = 0; c < channelCount; c++) {

    const auto compID = static_cast<ComponentID>(c);

    const ChannelType channelType = toChannelType(compID);
    const int bit_depth = internalBitDepth[channelType];
    const icov::video::QpParam cQP(qp, channelType, chFrmt, bit_depth, 0, 0);

    auto ldpc_args = m_parameters.coder_params->clone();

    ldpc_args->n = blockArea.block(compID).area();

    channelCoder.push_back(std::make_unique<decoder::ChannelDecoder>(
        decoder::ChannelDecoder::params{cQP, compID, bit_depth,
                                        maxLog2TrDynamicRange},
        *ldpc_args, m_vvc_bufs));
  }

  std::vector<int> bitplaneCount(channelCount);
  std::vector<NumcodeIdxVec> syndSizes(channelCount);

  icov::video::TransformUnit dec_tu(blockArea, true, true, true);

  //  PelStorage dec_block;
  //  dec_block.create(blockArea);

  inputFile.initBuffer(block_count, n);
  inputFile.readBlockPos();

  icov::helpers::assert::check(
      cps.is_block_fixed_size,
      "Only fixed size blocks is available in this version");
  //  icov::video::Quant quant;
  //  TCoeff uiAbsSum;
  //  icov::video::Transform2D transform2D;
  //  icov::video::IntraPrediction intraPred;

  EquiRectPicture icov_pic(cps.content_width, cps.content_height);
  IcovAccessBlockManager icov_accessblockMgr;
  ViewportManagerCenterPadded icov_viewport(1.2f);
  //  BlockPrediction icov_blockPrediction;

  icov::video::EncodingStructure encodingStruct;

  ICOV_DEBUG << "init decoding ICUs";

  encodingStruct
      .create360VideoInteractiveCodingUnits_withoutAccessBlockComputation(
          chFrmt, sourceWidth, sourceHeight, margin_x_360luma, icu_size,
          icu_size, icu_size, icu_size);

  // intraPred.init(chFrmt);

  icov_pic.buildConstantSizeBlocks(icu_size, icu_size);
  icov_accessblockMgr.buildAccessBlocks(icov_pic, icov_viewport);

  ICOV_DEBUG << "start decoding ICUs";

  for (int icu = 0; icu < block_count; ++icu) {
    ICOV_DEBUG << "decode block " << icu;

    bool is_access_block = (icu == 0);

    BlockPrediction::predType block_prediction =
        is_access_block ? BlockPrediction::predType::NONE
                        : BlockPrediction::predType::LEFT;

    BlockVector::const_iterator b = icov_pic.getById(icu + 1);

    int x_block = b->parent()->col(b->u());
    int y_block = b->parent()->row(b->v());

    Position pos(static_cast<float>(x_block) / static_cast<float>(icu_size),
                 static_cast<float>(y_block) / static_cast<float>(icu_size));

    const uint icuIndex = encodingStruct.get_icuIndex(pos);
    auto icu_enc = encodingStruct.getICU(icuIndex);

    inputFile.selectBlock(*b);
    inputFile.readSyndromesSizes(bitplaneCount, syndSizes);

    if (!is_access_block) {
      const int block_prediction_idx = getPredictionOrder(block_prediction);
      inputFile.selectPrediction(block_prediction_idx, bitplaneCount);
    }

    for (int c = 0; c < channelCount; c++) {

      channelCoder[c]->start_decode_icu(b, &icu_enc, block_prediction);

      const int lx = syndSizes[c].size();
      std::cout << "COMP " << c << " LX " << lx << std::endl;

      if (is_access_block) {
        channelCoder[c]->set_access_block_params(lx);
      } else {

        const int dir_mode = inputFile.readByteIntParam() - 1;

        std::cout << "SELECT PREDICTION " << block_prediction << ", "
                  << dir_mode << std::endl;

        ICOV_ASSERT_MSG(dir_mode >= 0, "Dir mode not available");

        const int pred_hdr = inputFile.readByteIntParam();
        int zmin = 0, zmax = 0, q_idx = 0;
        if (pred_hdr != 0) {
          inputFile.readQaryParams(pred_hdr, q_idx, zmin, zmax);
          ++q_idx;
        }

        channelCoder[c]->set_si_dir_mode(dir_mode);
        if (q_idx == 0) {
          channelCoder[c]->set_si_params(0, 0, 0, 0);
        } else {
          // ERROR NO CODING HERE !!!!
          const float q = m_q_values[q_idx - 1];
          std::cout << "QP " << zmin << ", " << zmax << ", " << q << std::endl;
          channelCoder[c]->set_si_params(zmin, zmax, q, lx);
        }
      }

      for (int bp = 0; bp < lx; ++bp) {
        std::cout << "BP " << bp + 1 << " / " << lx << std::endl;
        size_t stored_syndrome_size = channelCoder[c]->get_coder_size();
        size_t read_syndrome_size = channelCoder[c]->get_coder_size();
        icov::NumcodeIdx numcode = 1;

        if (!is_access_block) {
          numcode = inputFile.readByteIntParam();
          read_syndrome_size = channelCoder[c]->get_m(numcode);
          stored_syndrome_size = syndSizes[c][bp];
          std::cout << "SYND " << numcode << " " << read_syndrome_size << " "
                    << stored_syndrome_size << std::endl;
        }

        if (read_syndrome_size > 0) {
          inputFile.readSyndrome(channelCoder[c]->syndrome(),
                                 read_syndrome_size, stored_syndrome_size);

          std::cout << channelCoder[c]->syndrome().to_string(" ") << std::endl;
        }

        channelCoder[c]->decode_bitplane(bp, numcode,
                                         channelCoder[c]->syndrome());
      }

      // access block ?
      //      if (block_prediction == BlockPrediction::predType::NONE) {
      //        ICOV_TRACE << "Decode access block for comp " << c;
      //        for (int bp = 0; bp < lx; bp++) {
      //          inputFile.readSyndrome(blockBitplane, comp_area_value,
      //                                 comp_area_value);
      //          bsp.injectBitplane(bp, blockBitplane);
      //        }
      //      } else {
      //        ICOV_TRACE << "Decode block with prediction "
      //                   << BlockPrediction::toString(block_prediction)
      //                   << " for comp " << c;

      //        ICOV_ASSERT_MSG(si.TUs.size() == 1,
      //                        "This version of ICOV only supports one TU per
      //                        ICU");

      //        const int block_prediction_idx =
      //        getPredictionOrder(block_prediction);
      //        inputFile.selectPrediction(block_prediction_idx, channelCount);

      //        const int dir_mode = inputFile.readByteIntParam();
      //        const int pred_hdr = inputFile.readByteIntParam();

      //        PelBuf roiComp_predictionFrameBuff =
      //            VTM_Utility::getAreaBuf(predictionUnitBuf, comp_area);

      //        // std::cout << p << ", " << c << ": " << dir_mode << ", " <<
      //        pred_hdr
      //        //         << std::endl;

      //        if (pred_hdr != 0) {
      //        } else {

      //          // TODO
      //          bsp.set_si(bsp.si_symboles(), bsp.z_min(), bsp.z_max(),
      //          bsp.q0(),
      //                     bsp.x_binary_length());

      //          for (int bp = 0; bp < lx; bp++) {
      //            NumcodeIdx stored_synd_size = syndSizes[c][bp];
      //            NumcodeIdx pred_synd_size =
      //                static_cast<NumcodeIdx>(inputFile.readByteIntParam());

      //            // TODO CONVERT SIZE
      //            inputFile.readSyndrome(blockSyndrome, pred_synd_size,
      //                                   stored_synd_size);
      //          }
      //        }

      //        intraPred.initIntraPatternChType(reconstructionUnitBuffer,
      //                                         block_prediction, comp_area,
      //                                         bit_depth, false);

      //        ///
      //        home/sbelleno/_ICOV/CODE/adt.git/core/lib/icov/video_layer/Encodingstructure.cpp:1423
      //        ///
      //        ///
      //        si.intraDir[compID] = dir_mode;
      //        intraPred.predIntraAng(roiComp_predictionFrameBuff, dir_mode);
      //        auto &curr_tu_si = si.TUs[0];
      //        transform2D.transformNxN(curr_tu_si.getTransformCoeffs(compID),
      //                                 predictionUnitBuf,
      //                                 curr_tu_si.block(compID), bit_depth,
      //                                 maxLog2TrDynamicRange);
      //        quant.quant(curr_tu_si.getQuantizedTransformCoeffs(compID),
      //                    curr_tu_si.getTransformCoeffs(compID), uiAbsSum,
      //                    cQP, SliceType::I_SLICE, bit_depth,
      //                    maxLog2TrDynamicRange);
      //      }

      //      auto buf = dec_tu.getQuantizedTransformCoeffs(compID).buf;

      //      for (int i = 0; i < (int)bsp.src_symboles().size(); ++i) {
      //        // std::cout << i << std::endl;
      //        buf[i] = bsp.src_symboles()[i];
      //      }

      //      // iQ
      //      quant.dequant(dec_tu.getInverseQuantizedTransformCoeffs(compID),
      //                    dec_tu.getQuantizedTransformCoeffs(compID), cQP,
      //                    bit_depth, maxLog2TrDynamicRange);

      //      // iDCT
      //      transform2D.invTransformNxN(
      //          dec_block, dec_tu.block(compID),
      //          dec_tu.getInverseQuantizedTransformCoeffs(compID), bit_depth,
      //          maxLog2TrDynamicRange);
      //    }

      //      cv::Mat block_output;
      //      VTM_OPENCV_Utility::toBGRMat(dec_block.getBuf(blockArea),
      //      block_output);

      //      cv::imshow("DECODED", block_output);
      //      cv::waitKeyEx(0);

      channelCoder[c]->end_decode_icu();
    }
  }

  const char *windows_title = "decoded";
  cv::namedWindow(windows_title, cv::WINDOW_NORMAL);

  cv::Mat frame_output;
  VTM_OPENCV_Utility::toBGRMat(
      m_vvc_bufs->get(PIC_RECONSTRUCTION).getBuf(mainFrameArea), frame_output);

  cv::imshow(windows_title, frame_output);
  cv::waitKey(0);
}
