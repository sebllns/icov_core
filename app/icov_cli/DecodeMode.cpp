#include "DecodeMode.h"
#include "BitSymbolProvider.h"
#include "BlockPrediction.h"
#include "Common.h"
#include "common_types.h"
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
#include "video_layer/vtm_utility.h"

#include <algorithm>
#include <boost/format.hpp>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/serialization/vector.hpp>
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
      chFrmt, Area(0, 0, sourceWidth + 2 * margin_x_360luma, sourceHeight));
  const UnitArea mainFrameArea(
      chFrmt, Area(margin_x_360luma, 0, sourceWidth, sourceHeight));

  icov::video::BufGroups m_vvc_bufs;
  m_vvc_bufs.create(PIC_ORIGINAL, extendedFrameArea);
  m_vvc_bufs.create(PIC_RECONSTRUCTION, extendedFrameArea);
  m_vvc_bufs.create(PIC_PREDICTION, extendedFrameArea);

  const PelUnitBuf predictionUnitBuf = m_vvc_bufs.get(PIC_PREDICTION);
  const auto reconstructionUnitBuffer = m_vvc_bufs.getBuf(PIC_RECONSTRUCTION);

  std::vector<NumcodeIdxVec> syndSizes(channelCount);

  UnitArea blockArea(chFrmt, Area(0, 0, icu_size, icu_size));
  icov::video::TransformUnit dec_tu(blockArea, true, true, true);

  PelStorage dec_block;
  dec_block.create(blockArea);

  Syndrome blockSyndrome(n);
  BitVec blockBitplane(n);

  inputFile.initBuffer(block_count, n);

  icov::helpers::assert::check(
      cps.is_block_fixed_size,
      "Only fixed size blocks is available in this version");
  icov::video::Quant quant;
  TCoeff uiAbsSum;
  icov::video::Transform2D transform2D;
  icov::video::IntraPrediction intraPred;

  EquiRectPicture icov_pic(cps.content_width, cps.content_height);
  IcovAccessBlockManager icov_accessblockMgr;
  ViewportManagerCenterPadded icov_viewport(1.2f);
  BlockPrediction icov_blockPrediction;

  icov::video::EncodingStructure encodingStruct;

  encodingStruct
      .create360VideoInteractiveCodingUnits_withoutAccessBlockComputation(
          chFrmt, sourceWidth, sourceHeight, margin_x_360luma, icu_size,
          icu_size, icu_size, icu_size);

  intraPred.init(chFrmt);

  icov_pic.buildConstantSizeBlocks(icu_size, icu_size);
  icov_accessblockMgr.buildAccessBlocks(icov_pic, icov_viewport);

  for (int icu = 0; icu < block_count; ++icu) {
    bool is_access_block = (icu == 0);

    BlockPrediction::predType block_prediction =
        is_access_block ? BlockPrediction::predType::NONE
                        : BlockPrediction::predType::RIGHT;

    auto b = icov_pic.getById(0);

    int x_block = b->parent()->col(b->u());
    int y_block = b->parent()->row(b->v());

    Position pos(static_cast<float>(x_block) / static_cast<float>(icu_size),
                 static_cast<float>(y_block) / static_cast<float>(icu_size));

    auto &es = encodingStruct;
    const uint icuIndex = es.get_icuIndex(pos);
    auto icu_enc = es.getICU(icuIndex);

    auto currICUintraMap = icu_enc.get_intraSIsMap();

    inputFile.selectBlock(icu);
    inputFile.readSyndromesSizes(syndSizes);

    for (int p = 0; p < PREDICTION_COUNT; ++p) {
      inputFile.selectPrediction(p, channelCount);
      for (int c = 0; c < channelCount; c++) {
        // const ComponentID compID = ComponentID(c);
        // const auto compArea = icu_enc.get_sourceX().block(compID);

        int dir_mode = inputFile.readByteIntParam();
        int pred_hdr = inputFile.readByteIntParam();
        std::cout << p << ", " << c << ": " << dir_mode << ", " << pred_hdr
                  << std::endl;
        if (pred_hdr != 0) {
          int lx = 0, q_idx = 0, z_min = 0, z_max = 0;
          inputFile.readQaryParams(pred_hdr, q_idx, z_min, z_max);
          std::cout << lx << ", " << q_idx << ", " << z_min << ", " << z_max
                    << std::endl;
          NumcodeIdxVec nmc(lx, 0);
          inputFile.readSIRates(nmc);
          icov::helpers::io::write_vector(std::cout, nmc, ' ');
          std::cout << std::endl;
        }
      }
    }

    inputFile.selectBlock(icu);
    inputFile.readSyndromesSizes(syndSizes);
    for (int c = 0; c < channelCount; c++) {
      const auto compID = static_cast<ComponentID>(c);
      //      const CompArea &compArea = icu_enc.get_sourceX().block(compID);
      const auto comp_area = blockArea.block(compID);
      const auto comp_area_value = comp_area.area();

      const ChannelType channelType = toChannelType(compID);
      const int bit_depth = internalBitDepth[channelType];
      const icov::video::QpParam cQP(qp, channelType, chFrmt, bit_depth, 0, 0);

      BitSymbolProvider bsp(comp_area_value);
      int lx = syndSizes[c].size();
      std::cout << "LX " << lx << std::endl;

      // access block ?
      if (block_prediction == BlockPrediction::predType::NONE) {
        for (int bp = 0; bp < lx; bp++) {
          inputFile.readSyndrome(blockBitplane, comp_area_value,
                                 comp_area_value);
          bsp.injectBitplane(bp, blockBitplane);
        }
      } else {

        auto &si = currICUintraMap.at(block_prediction);

        ICOV_ASSERT_MSG(si.TUs.size() == 1,
                        "This version of ICOV only supports one TU per ICU");

        int block_prediction_idx = getPredictionOrder(block_prediction);
        auto stored_synd_size = syndSizes[c][lx];
        inputFile.selectPrediction(block_prediction_idx, channelCount);

        int dir_mode = inputFile.readByteIntParam();
        int pred_hdr = inputFile.readByteIntParam();

        PelBuf roiComp_predictionFrameBuff =
            VTM_Utility::getAreaBuf(predictionUnitBuf, comp_area);

        // std::cout << p << ", " << c << ": " << dir_mode << ", " << pred_hdr
        //         << std::endl;
        if (pred_hdr != 0) {
        }
        inputFile.readSyndrome(blockSyndrome, comp_area_value,
                               stored_synd_size);

        intraPred.initIntraPatternChType(reconstructionUnitBuffer,
                                         block_prediction, comp_area, bit_depth,
                                         false);

        /// home/sbelleno/_ICOV/CODE/adt.git/core/lib/icov/video_layer/Encodingstructure.cpp:1423
        ///
        ///
        si.intraDir[compID] = dir_mode;
        intraPred.predIntraAng(roiComp_predictionFrameBuff, dir_mode);
        //      for (UInt j = 0; j < n_tus; j++) {
        auto &curr_tu_si = si.TUs[0];
        transform2D.transformNxN(curr_tu_si.getTransformCoeffs(compID),
                                 predictionUnitBuf, curr_tu_si.block(compID),
                                 bit_depth, maxLog2TrDynamicRange);
        quant.quant(curr_tu_si.getQuantizedTransformCoeffs(compID),
                    curr_tu_si.getTransformCoeffs(compID), uiAbsSum, cQP,
                    SliceType::I_SLICE, bit_depth, maxLog2TrDynamicRange);
        //      }
      }

      auto buf = dec_tu.getQuantizedTransformCoeffs(compID).buf;

      for (int i = 0; i < (int)bsp.src_symboles().size(); ++i) {
        std::cout << i << std::endl;
        buf[i] = bsp.src_symboles()[i];
      }

      // iQ
      quant.dequant(dec_tu.getInverseQuantizedTransformCoeffs(compID),
                    dec_tu.getQuantizedTransformCoeffs(compID), cQP, bit_depth,
                    maxLog2TrDynamicRange);

      // iDCT
      transform2D.invTransformNxN(
          dec_block, dec_tu.block(compID),
          dec_tu.getInverseQuantizedTransformCoeffs(compID), bit_depth,
          maxLog2TrDynamicRange);

      // bsp.src_symboles()
      // dec_tu.getInverseQuantizedTransformCoeffs().buf;
    }

    cv::Mat block_output;
    VTM_OPENCV_Utility::toBGRMat(dec_block.getBuf(blockArea), block_output);

    cv::imshow("DECODED", block_output);
    cv::waitKeyEx(0);
  }
}
