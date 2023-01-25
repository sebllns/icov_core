#include "DecodeMode.h"
#include "BitSymbolProvider.h"
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

  const int maxLog2TrDynamicRange = 15;
  const Int internalBitDepth[MAX_NUM_CHANNEL_TYPE] = {8, 8};
  const ChromaFormat chFrmt = ChromaFormat::CHROMA_420;
  const int qp = 42;
  const int s = 32;
  const int n = s * s;
  const int channelCount = 3;
  const int block_count = 2;

  std::vector<NumcodeIdxVec> syndSizes(channelCount);

  inputFile.initBuffer(block_count, n);

  icov::helpers::assert::check(
      cps.is_block_fixed_size,
      "Only fixed size blocks is available in this version");
  icov::video::Quant quant;
  icov::video::Transform2D transform2D;

  for (int icu = 0; icu < block_count; ++icu) {
    inputFile.selectBlock(icu);
    inputFile.readSyndromesSizes(syndSizes);

    for (int p = 0; p < PREDICTION_COUNT; ++p) {
      inputFile.selectPrediction(p, channelCount);
      for (int c = 0; c < channelCount; c++) {
        int dir_mode = inputFile.readByteInt();
        int pred_hdr = inputFile.readByteInt();
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

    UnitArea blockArea(chFrmt, Area(0, 0, s, s));
    icov::video::TransformUnit dec_tu(blockArea, true, true, true);

    PelStorage dec_block;
    dec_block.create(blockArea);

    BitVec blockSyndrome(n);

    inputFile.selectBlock(icu);
    //  inputFile.selectPrediction(0, channelCount);
    inputFile.readSyndromesSizes(syndSizes);
    for (int c = 0; c < channelCount; c++) {
      const auto compID = static_cast<ComponentID>(c);
      const int nc = blockArea.block(compID).area();
      const ChannelType channelType = toChannelType(compID);
      const int bit_depth = internalBitDepth[channelType];
      const icov::video::QpParam cQP(qp, channelType, chFrmt, bit_depth, 0, 0);

      BitSymbolProvider bsp(nc);
      int lx = syndSizes[c].size();
      std::cout << "LX " << lx << std::endl;
      for (int bp = 0; bp < lx; bp++) {
        inputFile.readSyndrome(blockSyndrome, nc, nc);
        bsp.injectBitplane(bp, blockSyndrome);
      }

      //     intraPred.predIntraAng(roiComp_predictionFrameBuff,
      //     si.intraDir[compID]);
      /// home/sbelleno/_ICOV/CODE/adt.git/core/lib/icov/video_layer/Encodingstructure.cpp:1423
      ///
      ///       intraPred.predIntraAng(roiComp_predictionFrameBuff,
      ///       si.intraDir[compID]);
      //      for (UInt j = 0; j < n_tus; j++) {
      //        TransformUnit &curr_tu_si = si_TUs[j];
      //        transform2D.transformNxN(curr_tu_si.getTransformCoeffs(compID),
      //                                 predictionUnitBuf,
      //                                 curr_tu_si.block(compID),
      //                                 bitDepths[chType],
      //                                 maxLog2TrDynamicRange);
      //        quant.quant(curr_tu_si.getQuantizedTransformCoeffs(compID),
      //                    curr_tu_si.getTransformCoeffs(compID), uiAbsSum,
      //                    cQP, SliceType::I_SLICE, bitDepths[chType],
      //                    maxLog2TrDynamicRange);
      //      }

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

  EquiRectPicture icov_pic(cps.content_width, cps.content_height);
  IcovAccessBlockManager icov_accessblockMgr;
  ViewportManagerCenterPadded icov_viewport(1.2f);
  BlockPrediction icov_blockPrediction;

  int icu_size = cps.block_fixed_size;
  icov_pic.buildConstantSizeBlocks(icu_size, icu_size);
  icov_accessblockMgr.buildAccessBlocks(icov_pic, icov_viewport);
}
