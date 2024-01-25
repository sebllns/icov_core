#include "FrameEncoder.h"

#include "Buffer.h"
#include "CodingStructure.h"
#include "CommonDef.h"
#include "FastArray.h"
#include "enc_helpers.h"
#include "encoder/interface/IEncoder.h"
#include "icov_assert.h"
#include "icov_debug.h"
#include "icov_exception.h"
#include "icov_io.h"
#include "icov_log.h"
#include "icov_path.h"
#include "ldpc_codec_layer/BitSymbolProvider.h"
#include "ldpc_codec_layer/interface/IInteractiveCoder.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include "opencv2/imgcodecs.hpp"
#include "video_layer/ExtractedStreamDebugger.h"
#include "video_layer/encoded_stream.h"
#include "video_layer/extracted_stream.h"
#include "video_layer/icov_stream.h"
#include "video_layer/video_common.h"
#include "video_layer/vtm_opencv_utility.h"
#include "video_layer/vtm_utility.h"

#include <boost/algorithm/algorithm.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/file_status.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/format.hpp>

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <omp.h>
#include <sstream>
#include <string>
#include <vector>

namespace icov {
namespace encoder {
namespace fs = boost::filesystem;
using SI_SIDE = icov::omnidirectional_layer::BlockPrediction::ePredType;

/*****************************************************************************/
FrameEncoder::FrameEncoder(const params &p,
                           const LdpcCoderParameters &LDPC_Params)
    : m_params(p), m_numComp(getNumberValidComponents(p.ChFormat)),
      m_pred_encoder(
          LDPC_Params,
          std::vector<
              omnidirectional_layer::IBlockPredictionManager::prediction_t>{
              omnidirectional_layer::BlockPrediction::RIGHT,
              omnidirectional_layer::BlockPrediction::LEFT,
              omnidirectional_layer::BlockPrediction::TOP,
              omnidirectional_layer::BlockPrediction::BOTTOM,
              omnidirectional_layer::BlockPrediction::TOP_RIGHT,
              omnidirectional_layer::BlockPrediction::TOP_LEFT,
              omnidirectional_layer::BlockPrediction::BOTTOM_RIGHT,
              omnidirectional_layer::BlockPrediction::BOTTOM_LEFT,
              omnidirectional_layer::BlockPrediction::INTER}),
      m_init(true), block_comp(0) {}

/*****************************************************************************/
FrameEncoder::~FrameEncoder() {
  ICOV_TRACE << "Destroy FrameEncoder";
  m_stats.write(m_icov_file->stream());
  m_icov_file->endIcov();
}

/*****************************************************************************/
void icov::encoder::FrameEncoder::serializeSyndrome(bool access_block) {
  if (access_block) {
    if (m_params.ExportDebugTextFormat)
      m_icov_file->stream() << "access block data" << std::endl;
    for (int c = 0; c < (int)m_numComp; ++c) {
      auto &ch_coder = m_channelCoder[c];

      m_icov_file->setChannel(c);
      m_icov_file->startWritePrediction(
          omnidirectional_layer::BlockPrediction::ePredType::NONE);
      // store access block syndrome per bitplane
      Syndrome access_block_syndrome(ch_coder.source_block[0].size());

      int k = 0;
      for (const auto &s : ch_coder.source_block) {
        access_block_syndrome.fillFrom(s);
        if (m_params.ExportDebugTextFormat) {
          m_icov_file->stream() << "ch " << c << " bp " << k++ << " size "
                                << access_block_syndrome.size() << std::endl;
        }
        m_icov_file->writeSyndrome(access_block_syndrome);
      }
      m_icov_file->endWritePrediction(
          omnidirectional_layer::BlockPrediction::ePredType::NONE);
    }
  }

  if (m_params.ExportDebugTextFormat)
    m_icov_file->stream() << "syndromes" << std::endl;

  for (int c = 0; c < (int)m_numComp; ++c) {
    auto &ch_coder = m_channelCoder[c];

    m_icov_file->setChannel(c);
    // store normal syndrome per bitplane
    int k = 0;
    for (const auto &s : ch_coder.bitplanes) {
      if (m_params.ExportDebugTextFormat) {
        m_icov_file->stream() << "ch " << c << " bp " << k++ << " size "
                              << s.stored_synd.size() << std::endl;
      }
      if (s.stored_synd.size() > 0) {
        m_icov_file->writeSyndrome(s.stored_synd);
      }
    }
  }
}

/*****************************************************************************/
void icov::encoder::FrameEncoder::serializeParameters() {

  // Loop over each channel
  for (int c = 0; c < (int)m_numComp; ++c) {

    auto &ch_coder = m_channelCoder[c];

    m_icov_file->setChannel(c);
    // Store syndrome per bitplane
    m_icov_file->writeBitplaneCount(ch_coder.bitplanes.size());

    if (m_params.ExportDebugTextFormat) {
      m_icov_file->stream() << "channel " << c << std::endl;
      m_icov_file->stream()
          << "bitplane_count " << ch_coder.bitplanes.size() << std::endl;
      m_icov_file->stream() << "stored_syndrome_rates";
    }

    for (const auto &s : ch_coder.bitplanes) {
      m_icov_file->writeStoredSyndromeSize(
          s.stored_synd_rate.c, s.stored_synd_rate.r, s.stored_synd_rate.m);
      m_stats.addEncodedRate(s.stored_synd_rate.m);
      if (m_params.ExportDebugTextFormat) {
        m_icov_file->stream() << " " << s.stored_synd_rate.c << "("
                              << s.stored_synd_rate.m << ")";
      }
    }

    if (m_params.ExportDebugTextFormat) {
      m_icov_file->stream() << std::endl;
    }
  }

  for (omnidirectional_layer::IBlockPredictionManager::prediction_t p :
       bitstream::PredictionID) {

    if (m_params.ExportDebugTextFormat) {
      m_icov_file->stream()
          << "prediction "
          << omnidirectional_layer::BlockPrediction::toString(
                 omnidirectional_layer::BlockPrediction::toPredType(p))
          << std::endl;
    }

    for (int c = 0; c < (int)m_numComp; ++c) {

      auto &ch_coder = m_channelCoder[c];

      if (m_params.ExportDebugTextFormat) {
        m_icov_file->stream() << "channel " << c << std::endl;
      }

      m_icov_file->setChannel(c);
      m_icov_file->startWritePrediction(p);
      if (ch_coder.si.count(p)) {
        auto &si_coder = ch_coder.si.at(p);

        m_icov_file->writeSIMode(si_coder.dirmode);
        m_icov_file->writeQaryParams(si_coder.stored_pz);

        if (m_params.ExportDebugTextFormat) {
          m_icov_file->stream() << "mode " << si_coder.dirmode << std::endl;

          if (si_coder.stored_pz.codingflag()) {
            m_icov_file->stream()
                << "q " << si_coder.stored_pz.getRawQ0() << std::endl;
            m_icov_file->stream()
                << "zmin " << si_coder.stored_pz.getRawZmin() << std::endl;
            m_icov_file->stream()
                << "zmax " << si_coder.stored_pz.getRawZmax() << std::endl;
          } else {
            if (m_params.ExportDebugTextFormat)
              m_icov_file->stream() << "no coding" << std::endl;
          }
        }

        if (si_coder.stored_pz.codingflag()) {
          if (m_params.ExportDebugTextFormat) {
            m_icov_file->stream() << "rates";
          }

          for (int bp = 0; bp < (int)ch_coder.bitplanes.size(); ++bp) {

            if (ch_coder.bitplanes[bp].source_flag) {

              if (icov::debug::debug_trace_enabled) {
                ICOV_WARNING << "SOURCE IS STORED FOR CHANNEL " << c
                             << " BITPLANE " << bp;
              }

              m_icov_file->writeSIRate(
                  ldpc_codec_layer::IInteractiveCoder::SyndromeType_SOURCE, 1.0,
                  ch_coder.n);

              m_stats.addSIRate(ch_coder.n);

              if (m_params.ExportDebugTextFormat) {
                m_icov_file->stream() << " "
                                      << ldpc_codec_layer::IInteractiveCoder::
                                             SyndromeType::SyndromeType_SOURCE
                                      << "(" << ch_coder.n << ")";
              }

            } else {
              m_stats.addSIRate(si_coder.bitplanes[bp].m);

              m_icov_file->writeSIRate(si_coder.bitplanes[bp].c,
                                       si_coder.bitplanes[bp].r,
                                       si_coder.bitplanes[bp].m);

              if (m_params.ExportDebugTextFormat) {
                m_icov_file->stream() << " " << si_coder.bitplanes[bp].c << "("
                                      << si_coder.bitplanes[bp].m << ")";
              }
            }
          }
          if (m_params.ExportDebugTextFormat) {
            m_icov_file->stream() << std::endl;
          }
        }
      } else if (p != omnidirectional_layer::BlockPrediction::ePredType::NONE) {
        m_icov_file->writeSIMode(bitstream::flag_t::UnavailableSI);
        if (m_params.ExportDebugTextFormat) {
          m_icov_file->stream()
              << "mode " << (int)bitstream::flag_t::UnavailableSI << std::endl;
        }
      }
      m_icov_file->endWritePrediction(p);
    }
  }
}

/*****************************************************************************/
void icov::encoder::FrameEncoder::serializeBlockData(
    bool access_block, const omnidirectional_layer::Block &b) {

  ICOV_DEBUG << "Serialize block " << b.getIdSelf();

  if (m_params.ExportDebugTextFormat) {
    m_icov_file->stream() << "block " << b.getIdSelf() << std::endl;
  }

  // Start writing block data
  m_icov_file->startWriteBlock(b);

  // WRITE SYNDROMES
  serializeSyndrome(access_block);

  // Store bitplanes rates per SI and per channel
  serializeParameters();

  m_icov_file->writeBlockSize();

  // End writing block data
  m_icov_file->endWriteBlock(b);

  if (m_params.ExportDebugTextFormat) {
    fs::path block_data_dbg_path = m_icov_file->getPath().parent_path() /
                                   "debug" / std::to_string(frame());
    fs::create_directories(block_data_dbg_path);

    block_data_dbg_path /= std::to_string(b.getIdSelf());
    std::ofstream ofblock(block_data_dbg_path);

    /*****************************************************************************/

    // Loop over each channel
    for (int c = 0; c < (int)m_numComp; ++c) {
      auto &ch_coder = m_channelCoder[c];

      ofblock << "channel: " << c << std::endl;

      ofblock << "bitplane_count: " << ch_coder.bitplanes.size() << std::endl;

      ofblock << "stored_syndrome_rates: ";
      for (const auto &s : ch_coder.bitplanes) {
        ofblock << " " << s.stored_synd_rate.c << "(" << s.stored_synd_rate.m
                << ")";
      }

      if (m_params.ExportDebugTextFormat) {
        ofblock << std::endl;
      }
    }

    for (omnidirectional_layer::IBlockPredictionManager::prediction_t p :
         bitstream::PredictionID) {

      ofblock << "prediction: "
              << omnidirectional_layer::BlockPrediction::toString(
                     omnidirectional_layer::BlockPrediction::toPredType(p))
              << std::endl;

      for (int c = 0; c < (int)m_numComp; ++c) {

        auto &ch_coder = m_channelCoder[c];

        ofblock << "channel: " << c << std::endl;

        if (ch_coder.si.count(p)) {
          auto &si_coder = ch_coder.si.at(p);

          ofblock << "mode: " << si_coder.dirmode << std::endl;

          if (si_coder.stored_pz.codingflag()) {
            ofblock << "q " << si_coder.stored_pz.getRawQ0() << std::endl;
            ofblock << "zmin " << si_coder.stored_pz.getRawZmin() << std::endl;
            ofblock << "zmax " << si_coder.stored_pz.getRawZmax() << std::endl;
          } else {
            if (m_params.ExportDebugTextFormat)
              ofblock << "no coding" << std::endl;
          }

          if (si_coder.stored_pz.codingflag()) {
            ofblock << "rates";

            for (int bp = 0; bp < (int)ch_coder.bitplanes.size(); ++bp) {

              if (ch_coder.bitplanes[bp].source_flag) {

                ofblock << " "
                        << ldpc_codec_layer::IInteractiveCoder::SyndromeType::
                               SyndromeType_SOURCE
                        << "(" << ch_coder.n << ")";

              } else {

                ofblock << " " << si_coder.bitplanes[bp].c << "("
                        << si_coder.bitplanes[bp].m << ")";
              }
            }
            ofblock << std::endl;
          }
        } else if (p !=
                   omnidirectional_layer::BlockPrediction::ePredType::NONE) {
          ofblock << "mode " << (int)bitstream::flag_t::UnavailableSI
                  << std::endl;
        }
      }
    }
  }
}

/*****************************************************************************/
void FrameEncoder::encodeBlock(
    const omnidirectional_layer::Block &b, bool access_block,
    const icov::omnidirectional_layer::IBlockPredictionManager
        &block_prediction_mgr) {

  m_stats.addBlock();

  //  ICOV_DEBUG << "start encode block "
  //             << b.getId(omnidirectional_layer::Block::SELF);

  // RESET encoded data for the block
  m_channelCoder = std::vector<ChannelCoder>(m_numComp);

  auto v = getAvailablePredictions(b, access_block, block_prediction_mgr);
  v.push_back(icov::omnidirectional_layer::BlockPrediction::ePredType::INTER);

  for (int c = 0; c < (int)m_numComp; ++c) {
    block_comp = c;
#pragma omp parallel for
    for (omnidirectional_layer::IBlockPredictionManager::prediction_t p : v)
      encodeBlockWithPrediction(b, p);
  }

  serializeBlockData(access_block, b);

  ICOV_DEBUG << "end encode block "
             << b.getId(omnidirectional_layer::Block::SELF);
}

/*****************************************************************************/
void FrameEncoder::processInput(const IInputMedia<cv::Mat> &input) {
  m_stats.addFrame();

  bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingFrameID(
      frame());

  //  ICOV_ASSERT_MSG(!ec.failed(), "Error create directory");

  cv::Mat img_src = input.getCurrentFrame();

  ICOV_ASSERT_MSG(img_src.total() > 0, "Cannot encode empty image");

  // CONSTANTS
  const Size icuSize_luma =
      Size(m_params.ICU_Size.width, m_params.ICU_Size.height);

  // end constants

  // resize
  //  cv::resize(img_src, img_src, cv::Size(new_w, new_h));

  const cv::Size size_before = img_src.size();
  resize_covering_blocks(img_src,
                         cv::Size(icuSize_luma.width, icuSize_luma.height));
  const cv::Size size_after = img_src.size();

  if (size_after != size_before) {
    ICOV_WARNING << "Image resized from " << size_before << " to "
                 << size_after;

    ICOV_WARNING << "Aspect changed from "
                 << double(size_before.width) / double(size_before.height)
                 << " to "
                 << double(size_after.width) / double(size_after.height);
  }

  const int sourceWidth = img_src.cols;
  const int sourceHeight = img_src.rows;
  // end resize

  const UnitArea extendedFrameArea(
      m_params.ChFormat,
      Area(0, 0, sourceWidth + 2 * m_params.MarginX360Luma, sourceHeight));
  const UnitArea mainFrameArea(
      m_params.ChFormat,
      Area(m_params.MarginX360Luma, 0, sourceWidth, sourceHeight));

  if (m_init) {
    m_vvc_bufs.create(PIC_ORIGINAL, extendedFrameArea);
    m_vvc_bufs.create(PIC_RECONSTRUCTION, extendedFrameArea);
    m_vvc_bufs.create(PIC_PREDICTION, extendedFrameArea);

    std::vector<omnidirectional_layer::BlockPrediction::ePredType> predVec = {
        omnidirectional_layer::BlockPrediction::RIGHT,
        omnidirectional_layer::BlockPrediction::LEFT,
        omnidirectional_layer::BlockPrediction::TOP,
        omnidirectional_layer::BlockPrediction::BOTTOM,
        omnidirectional_layer::BlockPrediction::TOP_LEFT,     // 0b00101
        omnidirectional_layer::BlockPrediction::BOTTOM_LEFT,  // 0b01001
        omnidirectional_layer::BlockPrediction::BOTTOM_RIGHT, // 0b01010
        omnidirectional_layer::BlockPrediction::TOP_RIGHT};

    for (auto pk : predVec) {
      m_vvc_bufs.create_link(PIC_PREDICTION + pk, PIC_PREDICTION);
    }

    m_enc_struc
        .create360VideoInteractiveCodingUnits_withoutAccessBlockComputation(
            m_params.ChFormat, sourceWidth, sourceHeight,
            m_params.MarginX360Luma, icuSize_luma.width, icuSize_luma.height,
            icuSize_luma.width, icuSize_luma.height);

    m_init = false;
  }

  m_vvc_bufs.get(PIC_ORIGINAL).fill(0);
  m_vvc_bufs.get(PIC_RECONSTRUCTION).fill(0);
  m_vvc_bufs.get(PIC_PREDICTION).fill(0);

  PelUnitBuf mainFrameOrig = m_vvc_bufs.get(PIC_ORIGINAL).getBuf(mainFrameArea);

  ICOV_ASSERT_MSG((int)(mainFrameOrig.get(COMPONENT_Y).width) == img_src.cols,
                  "Width does not match");

  ICOV_ASSERT_MSG((int)(mainFrameOrig.get(COMPONENT_Y).height) == img_src.rows,
                  "Height does not match");

  VTM_OPENCV_Utility::cvMatToYUV(img_src, mainFrameOrig);

  VTM_Utility::quantizeFrame(
      m_params.QP, m_vvc_bufs.get(PIC_ORIGINAL),
      m_vvc_bufs.get(PIC_RECONSTRUCTION), m_enc_struc,
      m_params.InternalBitDepth,
      m_params.MaxLog2TrDynamicRange); // give the whole extended image (
                                       // .getOrigBuf()
  // and
  // .getRecoBuf() ) to the function because vector
  // icues_enc is based on extended frames

  // INTRA PRED
  PelUnitBuf mainFrameReco =
      m_vvc_bufs.get(PIC_RECONSTRUCTION).getBuf(mainFrameArea);

  ICOV_DEBUG << "Start computing intra predictions";
  for (UInt ch = 0; ch < m_numComp; ch++) {
    ICOV_DEBUG << "Channel " << ch + 1 << " / " << m_numComp;

    const ComponentID compID = ComponentID(ch);
    VTM_Utility::copyBordersFor360(mainFrameReco, compID);

    m_enc_struc.applyPredictionBasedOnEntropy(
        m_vvc_bufs, compID, m_params.InternalBitDepth,
        m_params.TU_SizeAfterRegroupingLuma.width,
        m_params.TU_SizeAfterRegroupingLuma.height, false, m_params.QP,
        m_params.MaxLog2TrDynamicRange);
  }
  ICOV_DEBUG << "Successfully computed intra predictions";

  // END INTRA PRED

  // compute access block
  VTM_OPENCV_Utility::toBGRMat(mainFrameReco, m_reco);

  for (const auto &icu : m_enc_struc.vec_icus()) {
    double mse = 0., psnr = 0.;
    uint max_val = 0;

    const CPelUnitBuf b_orig =
        m_vvc_bufs.get(PIC_ORIGINAL).getBuf(icu.get_sourceX());
    const CPelUnitBuf b_reco =
        m_vvc_bufs.get(PIC_RECONSTRUCTION).getBuf(icu.get_sourceX());

    psnr = VTM_Utility::calculatePSNR(b_orig, b_reco, m_params.InternalBitDepth,
                                      &mse, &max_val);

    // write mse into file
    m_icov_file->writeMse(mse);
  }

  m_icov_file->contentParameterSet().content_framecount++;

  if (m_params.ExportRecoFrames) {
    cv::imwrite(path::combine(m_framesPath.string(),
                              "x" + std::to_string(frame()) + ".tif"),
                m_reco);

    cv::imwrite(path::combine(m_framesPath.string(),
                              "v" + std::to_string(frame()) + ".tif"),
                img_src);
  }

  if (m_params.ExportDebugTextFormat) {
    m_icov_file->stream() << "frame " << frame() << std::endl;
  }
}

/*****************************************************************************/
void icov::encoder::FrameEncoder::checkDecoding(
    ldpc_codec_layer::AffInteractiveCoder &encoder,
    const ldpc_codec_layer::BitSymbolProvider &bsp_enc,
    const icov::NumcodeIdxVec &si_numcodes, const icov::SyndromesVec &si_synd,
    ComponentID comp) {

  ldpc_codec_layer::BitSymbolProvider bsp_dec(encoder.get_n());

  bsp_dec.set_si_and_qaryDistribution(bsp_enc.si_symboles(),
                                      bsp_enc.distribution());

  for (size_t i = 0; i < si_numcodes.size(); i++) {
    BitVec decoded;
    try {
      ICOV_TRACE << "Check bitplane " << i;
      encoder.decode_bitplane(decoded, i, bsp_dec, si_numcodes[i], si_synd[i]);
      icov::assert::check(decoded == bsp_enc.src_bits()[i],
                          "Decoding BP check failed " + std::to_string(i));
    } catch (const std::exception &e) {
      ICOV_ERROR << e.what();
      break;
    }
  }

  try {
    bsp_dec.set_si_and_qaryDistribution(bsp_enc.si_symboles(),
                                        bsp_enc.distribution());
    encoder.decode_symboles(bsp_dec, si_numcodes, si_synd);

    icov::assert::check(bsp_dec.src_symboles() == bsp_enc.src_symboles(),
                        "Decoding symbole failed for channel " +
                            std::to_string(comp));
  } catch (const std::exception &e) {
    ICOV_ERROR << e.what();
  }
}

/*****************************************************************************/
void icov::encoder::FrameEncoder::saveDataToEncode(
    bool access_block, uint mode,
    ldpc_codec_layer::AffInteractiveCoder &encoder,
    const ldpc_codec_layer::BitSymbolProvider &bsp_enc,
    const icov::NumcodeIdxVec &si_numcodes, const icov::SyndromesVec &si_synd,
    icov::omnidirectional_layer::BlockPrediction::ePredType pred,
    ComponentID comp) {

  // Get the number of bitplanes to be saved
  int bitplaneCount = bsp_enc.bitplane_count();

  const auto pred_id =
      static_cast<omnidirectional_layer::IBlockPredictionManager::prediction_t>(
          pred);

  bool is_inter =
      (pred == icov::omnidirectional_layer::BlockPrediction::ePredType::INTER);

  if (is_inter && frame() == 0) {
    ICOV_THROW_LOGICAL_EXCEPTION(
        "Cannot encoder inter prediction on the first frame");
  }

  // Store information related to the current prediction in the channel coder
  auto &ch_coder = m_channelCoder[comp];

  // First SI stored (no bitplanes stored for comparison yet)
  if (ch_coder.bitplanes.size() == 0) {
    ch_coder.n = encoder.get_n();

    // store source bits
    ch_coder.source_block = bsp_enc.src_bits();

    ch_coder.bitplane_count = bitplaneCount;

    // Initialize bitplanes vector with default values
    ch_coder.bitplanes.resize(bitplaneCount);

    // Populate bitplanes information with syndromes and rates
    for (size_t i = 0; i < si_synd.size(); ++i) {

      ch_coder.bitplanes[i].stored_synd_rate.c = si_numcodes[i];

      ch_coder.bitplanes[i].stored_synd_rate.m =
          bitstream::inverseQuantizedSize(si_numcodes[i], encoder);

      ch_coder.bitplanes[i].stored_synd_rate.r = encoder.get_r(
          ch_coder.bitplanes[i].stored_synd_rate.m, encoder.get_n());

      ch_coder.bitplanes[i].stored_synd = si_synd[i];
      ch_coder.bitplanes[i].source_flag = false;
    }
  }

  if (ch_coder.si.count(pred_id)) {
    ICOV_THROW_LOGICAL_EXCEPTION(
        "Prediction has already been encoded: " +
        omnidirectional_layer::BlockPrediction::toString(pred));
  }

  auto &si_coder = ch_coder.addSi(pred_id);

  si_coder.stored_pz = bsp_enc.distribution();
  si_coder.dirmode = mode;

  if ((int)ch_coder.bitplane_count != bitplaneCount)
    ICOV_THROW_LOGICAL_EXCEPTION("Bad bitplane count");

  if ((int)ch_coder.bitplanes.size() != bitplaneCount)
    ICOV_THROW_LOGICAL_EXCEPTION("Bad bitplane count");

  // If coding flag is true for this SI
  if (bsp_enc.distribution().codingflag()) {

    if ((int)si_synd.size() != bitplaneCount)
      ICOV_THROW_LOGICAL_EXCEPTION("Bad bitplane count");

    if ((int)si_numcodes.size() != bitplaneCount)
      ICOV_THROW_LOGICAL_EXCEPTION("Bad bitplane count");

    // Assert bitplane count consistency and SI sizes
    icov::assert::check(
        (int)ch_coder.bitplanes.size() == bitplaneCount &&
            si_synd.size() == si_numcodes.size(),
        ICOV_DEBUG_HEADER(
            "Bitplane count does not match between SI of same block"));

    if ((int)si_coder.bitplanes.size() != bitplaneCount)
      ICOV_THROW_LOGICAL_EXCEPTION("Bad bitplane count");

    // Loop through each bitplane
    for (int i = 0; i < bitplaneCount; ++i) {

      BitplaneCoder &bp_coder = ch_coder.bitplanes[i];
      StoredRate &si_bp_rate = si_coder.bitplanes[i];

      bool &source_flag = bp_coder.source_flag;

      auto synd_type =
          ldpc_codec_layer::IInteractiveCoder::getSyndromeType(si_numcodes[i]);
      bool no_coding =
          (synd_type ==
           ldpc_codec_layer::IInteractiveCoder::SyndromeType_NO_CODING);
      bool source_coding =
          (synd_type ==
           ldpc_codec_layer::IInteractiveCoder::SyndromeType_SOURCE) ||
          source_flag;
      bool bitplane_coding =
          (synd_type ==
           ldpc_codec_layer::IInteractiveCoder::SyndromeType_NORMAL);

      bool intra_source = !is_inter && source_coding;
      bool inter_source = is_inter && source_coding;

      // if NO coding flag is TRUE for this BITPLANE
      if (no_coding) {
        si_bp_rate.c = si_numcodes[i];
        si_bp_rate.r = 0.0f;
        si_bp_rate.m = 0;

        ICOV_TRACE << "RATE FOR BP " << i << " PREDICTION "
                   << omnidirectional_layer::BlockPrediction::toString(pred)
                   << ": 0 (NO CODING)";

      }
      // IF SOURCE FLAG is TRUE for the BITPLANE
      else if (source_flag || intra_source) {

        if (!source_flag) {
          source_flag = true;
          bp_coder.stored_synd.fillFrom(bsp_enc.src_bits()[i]);
          bp_coder.stored_synd_rate.c =
              ldpc_codec_layer::IInteractiveCoder::SyndromeType_SOURCE;
          bp_coder.stored_synd_rate.m = encoder.get_n();
          bp_coder.stored_synd_rate.r = 1.0f;
        }

        si_bp_rate.c = ldpc_codec_layer::IInteractiveCoder::SyndromeType_SOURCE;
        si_bp_rate.m = encoder.get_n();
        si_bp_rate.r = 1.0f;

        ICOV_TRACE << "RATE FOR BP " << i << " PREDICTION "
                   << omnidirectional_layer::BlockPrediction::toString(pred)
                   << ": SOURCE (NO CODING)";

      }
      // NORMAL CASE : bitplane was normally encoded
      else if (is_inter || bitplane_coding) {

        si_bp_rate.c = si_numcodes[i];
        si_bp_rate.m = bitstream::inverseQuantizedSize(si_bp_rate.c, encoder);
        si_bp_rate.r = encoder.get_r(si_bp_rate.m, encoder.get_n());

        ICOV_TRACE << "RATE FOR BP " << i << " PREDICTION "
                   << omnidirectional_layer::BlockPrediction::toString(pred)
                   << ": " << si_bp_rate.r << ", " << si_bp_rate.m << ", "
                   << si_bp_rate.c;

        if (!is_inter) {

          //          if (bp_coder.noSourceThreshold >
          //                  m_params.MaxRateNoCoding &&
          //              !source_flag) {
          //            source_flag = true;
          //            bp_coder.stored_synd.fillFrom(
          //                bsp_enc.src_bits()[i]);

          //            bp_coder.stored_synd_rate.c =
          //                ldpc_codec_layer::IInteractiveCoder::SyndromeType_SOURCE;
          //            bp_coder.stored_synd_rate.m =
          //                encoder.get_n();
          //            bp_coder.stored_synd_rate.r
          //            = 1.0;

          //            si_bp_rate.c
          //            =
          //                ldpc_codec_layer::IInteractiveCoder::SyndromeType_SOURCE;
          //            si_bp_rate.m
          //            =
          //                encoder.get_n();
          //            si_bp_rate.r
          //            = 1.0f;

          //            ICOV_WARNING
          //                << "Too many predictions have a very long syndrome,
          //                so we take "
          //                   "decision to store the source instead for
          //                   bitplane "
          //                << i << " at channel " << comp; /*STORED SOURCE*/
          //          }
        }

        if (!source_flag && bitplane_coding &&
            si_synd[i].size() > bp_coder.stored_synd.size())
        /*STORED SYNDROME IF LONGER*/ {
          bp_coder.stored_synd = si_synd[i];
          bp_coder.stored_synd_rate = si_bp_rate;
        }
      }
      // ABNORMAL CASE : send ERROR
      else {
        ICOV_THROW_LOGICAL_EXCEPTION(
            "Bad rate value for bitplane " + std::to_string(i) +
            ": rate quantized value is " + std::to_string(si_numcodes[i]));
      }
    }
  }
}

/*****************************************************************************/
void icov::encoder::FrameEncoder::saveDataToDebug(
    omnidirectional_layer::Block b,
    ldpc_codec_layer::AffInteractiveCoder &encoder,
    const ldpc_codec_layer::BitSymbolProvider &bsp_enc,
    const icov::NumcodeIdxVec &si_numcodes, const icov::SyndromesVec &si_synd,
    icov::omnidirectional_layer::BlockPrediction::ePredType pred,
    ComponentID comp) {

  ICOV_DEBUG << "Save debug for " << b.getIdSelf() << ", "
             << icov::omnidirectional_layer::BlockPrediction::toString(pred)
             << ", " << (int)comp;

  auto &ch_coder = m_channelCoder[comp];

  int bitplaneCount = bsp_enc.bitplane_count();
  const auto pred_id =
      static_cast<omnidirectional_layer::IBlockPredictionManager::prediction_t>(
          pred);

  if (!ch_coder.si.count(pred_id))
    return;

  auto &si_coder = ch_coder.si.at(pred_id);

  bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingBlockID(
      b.getIdSelf() - 1);
  bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingChannelID(
      comp);

  auto &channelDbg = bitstream::VideoStreamDebugger::get_mutable_instance()
                         .getDebuggingChannel();

  if (channelDbg.n() == 0) {
    channelDbg.srcSymboles = bsp_enc.src_symboles();
    channelDbg.srcBits = bsp_enc.src_bits();
  }

  channelDbg.storedSyndromeSize.resize(ch_coder.bitplanes.size());
  std::transform(ch_coder.bitplanes.cbegin(), ch_coder.bitplanes.cend(),
                 channelDbg.storedSyndromeSize.begin(),
                 [](const auto &x) { return x.stored_synd_rate.c; });

  // Checks if the block is an access block and there is no saved data
  if (b.isAccessBlock() &&
      channelDbg.si_map.count(
          omnidirectional_layer::BlockPrediction::ePredType::NONE) == 0) {

    auto &si_dbg =
        channelDbg
            .si_map[omnidirectional_layer::BlockPrediction::ePredType::NONE];

    si_dbg.mode = bitstream::flag_t::AccessBlock;
    si_dbg.pz = ldpc_codec_layer::QaryDistribution();
    si_dbg.pz.setBitplane_count(ch_coder.source_block.size());

    for (const auto &s : ch_coder.source_block) {
      bitstream::BitplaneStream bpstr;
      bpstr.numcode = ldpc_codec_layer::IInteractiveCoder::SyndromeType_SOURCE;
      bpstr.syndrome.fillFrom(s);
      si_dbg.bitplanes.push_back(bpstr);
    }
  } // end access block

  bitstream::VideoStreamDebugger::get_mutable_instance()
      .setDebuggingPredictionID(pred);

  // checks if there is no existing debug for this prediction and throws an
  // error if there is
  icov::assert::check(!bitstream::VideoStreamDebugger::get_const_instance()
                           .predictionAvailable(),
                      ICOV_DEBUG_HEADER("Prediction debug already exists"));

  auto &si_dbg = channelDbg.si_map[pred];

  si_dbg.values = bsp_enc.si_symboles();
  si_dbg.mode = si_coder.dirmode;
  si_dbg.pz = si_coder.stored_pz;

  ICOV_DEBUG << "Mode is " << (int)si_dbg.mode;

  if (si_dbg.pz.codingflag()) {
    // adds each bitplane for the given prediction to the debugging
    // bitplanes vector
    si_dbg.bitplanes.resize(bitplaneCount);
    for (int i = 0; i < bitplaneCount; ++i) {
      // set numcode (quantized rate)
      bitstream::BitplaneStream bpstr;

      // if source coding set it to all SI
      if (ch_coder.bitplanes[i].source_flag) {
        bpstr.numcode =
            ldpc_codec_layer::IInteractiveCoder::SyndromeType_SOURCE;
        bpstr.syndrome = ch_coder.bitplanes[i].stored_synd;
        for (auto &s : channelDbg.si_map) {
          s.second.bitplanes.resize(bitplaneCount);
          if (s.second.bitplanes[i].numcode !=
              ldpc_codec_layer::IInteractiveCoder::SyndromeType_SOURCE) {
            s.second.bitplanes[i] = bpstr;
          }
        }
      } else {
        bpstr.numcode = si_coder.bitplanes[i].c;

        // set syndrome
        if (bpstr.numcode ==
                ldpc_codec_layer::IInteractiveCoder::SyndromeType_NO_CODING

            || (bpstr.numcode ==
                    ldpc_codec_layer::IInteractiveCoder::SyndromeType_SOURCE &&
                pred == omnidirectional_layer::BlockPrediction::INTER)) {
          bpstr.syndrome = Syndrome();
        } else if (bpstr.numcode ==
                   ldpc_codec_layer::IInteractiveCoder::SyndromeType_SOURCE) {
          ICOV_THROW_LOGICAL_EXCEPTION(
              "The case where bitplane is source coding "
              "should have been treated before");
        } else {
          bpstr.syndrome = si_synd[i];
        }

        // assign bitplane stream in debug si
        si_dbg.bitplanes[i] = bpstr;
      }

      ICOV_DEBUG << "Save in debug bp " << i << " for prediction "
                 << icov::omnidirectional_layer::BlockPrediction::toString(pred)
                 << " for channel " << comp << " numcode " << bpstr.numcode;
    }
  } else {
    // no bitplane
    si_dbg.bitplanes.resize(0);
  }
}

/*****************************************************************************/
void icov::encoder::FrameEncoder::encode_tu(
    const icov::omnidirectional_layer::Block &b,
    const icov::video::IcuEncoder &icu_enc,
    const icov::video::SideInformationUnit *si,
    icov::omnidirectional_layer::BlockPrediction::ePredType pred,
    ComponentID comp) {

  // Initialize TU index and prediction identifier
  int tu_index = 0;
  const auto pred_id =
      static_cast<omnidirectional_layer::IBlockPredictionManager::prediction_t>(
          pred);

  // Get the TU and its quantized transform coefficients
  const auto &tu = icu_enc.get_sourceX().TUs[tu_index];
  auto tu_coeff = tu.getQuantizedTransformCoeffs(comp);
  size_t tu_size = tu_coeff.area();
  auto &encoder = m_pred_encoder.get_coder(pred_id, tu_size);

  // encode outputs for the current SI: vector of rates and vector of syndrome
  // (one rate and syndrome per bitplane)
  icov::NumcodeIdxVec si_numcodes;
  icov::SyndromesVec si_synd;

  // Extract the quantized transform coefficients side info for encoding
  const CCoeffBuf &si_tu = (si == nullptr)
                               ? tu.getPreviousQT(comp) /* inter prediction*/
                               : si->TUs[tu_index].getQuantizedTransformCoeffs(
                                     comp) /* intra prediction */;

  // Instantiate Bit Symbol Provider for encoding
  ldpc_codec_layer::BitSymbolProvider bsp_enc =
      tu.instantiateBitSymbolProvider(comp, si_tu, m_params.BSP_Params);

  //  if (si == nullptr) {
  //    m_icov_file->stream() << "INTER DIFF "
  //                          << std::to_string(b.getIdSelf()) + "_" +
  //                                 std::to_string(frame())
  //                          << "_" << std::to_string((int)comp) << " = ";
  //    icov::io::write_vector(m_icov_file->stream(), bsp_enc.getDiff(), ' ');
  //    m_icov_file->stream() << std::endl;
  //  }

  // If coding flag is false, no encoding is performed
  if (!bsp_enc.distribution().codingflag()) { /* CODING FLAG IS FALSE */
    ICOV_TRACE << "SI symbole identical... no coding...";
  } else { /* CODING FLAG IS TRUE */
    // Coding flag is true, perform encoding
    // Perform bitplane encoding
    encoder.encode_bitplanes(bsp_enc, si_numcodes, si_synd);

    // Check that decoding works without error if the test is enabled in
    // parameters
    if (ldpc_codec_layer::IInteractiveCoder::params.CheckBitplaneEncoding) {
      checkDecoding(encoder, bsp_enc, si_numcodes, si_synd, comp);
    }
    // END CHECK DECODING
  }
// encoding and decoding check done, now save syndrome if longer than the
// current one saved
#pragma omp critical
  {
    uint mode = 0;

    if (pred != omnidirectional_layer::BlockPrediction::INTER) {
      mode = icu_enc.get_intraSIsMap().at(pred).intraDir[block_comp];
    } else {
      mode = NUM_LUMA_MODE; // special value for INTER prediction
    }

    mode += bitstream::offset_t::AvailableSI;

    // Save syndrome if longer than current saved
    saveDataToEncode(b.isAccessBlock(), mode, encoder, bsp_enc, si_numcodes,
                     si_synd, pred, comp);

    // Save data to debug if debugging is enabled
    if (debug_enabled()) {
      saveDataToDebug(b, encoder, bsp_enc, si_numcodes, si_synd, pred, comp);
    }
  }
  /*#pragma omp critical*/
}

bool FrameEncoder::debug_enabled() const {
  bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingFrameID(
      frame());
  return bitstream::VideoStreamDebugger::get_const_instance().frameAvailable();
}

const std::shared_ptr<OutputIcovBase> &FrameEncoder::icovFile() const {
  return m_icov_file;
}

void FrameEncoder::setIcovFile(
    const std::shared_ptr<OutputIcovBase> &newIcov_file) {
  m_icov_file = newIcov_file;

  icov::assert::not_null(m_icov_file.get(),
                         ICOV_DEBUG_HEADER("Cannot encode into NULL file"));

  if (m_params.ExportRecoFrames) {
    m_framesPath =
        path::combine(m_icov_file->getDir().string(), "source_frames");
    fs::create_directories(m_framesPath);
  }
}

void icov::encoder::FrameEncoder::encode_si(
    const omnidirectional_layer::Block &b,
    const icov::video::IcuEncoder &icu_enc,
    const icov::video::SideInformationUnit *si,
    omnidirectional_layer::BlockPrediction::ePredType pred, ComponentID comp) {

  icov::assert::equal((int)icu_enc.sourceX_TUs().size(), 1,
                      "This version of ICOV can only manage one transform unit "
                      "(TU) per block (ICU)");

  if (pred == omnidirectional_layer::BlockPrediction::ePredType::INTER) {
    ICOV_ASSERT_MSG(si == nullptr, "Inter prediction must not carry SI unit");
  } else {
    ICOV_ASSERT_MSG(si != nullptr, "Intra prediction must carry SI unit");
  }

  encode_tu(b, icu_enc, si, pred, comp);
}

void icov::encoder::FrameEncoder::encode_icu(
    const omnidirectional_layer::Block &b,
    const icov::video::IcuEncoder &icu_enc,
    omnidirectional_layer::BlockPrediction::ePredType pred) {

  auto si = icu_enc.get_sideInformation(pred);

  if (pred != icov::omnidirectional_layer::BlockPrediction::ePredType::INTER) {
    icov::assert::not_null(si);
  }

  encode_si(b, icu_enc, si, pred, static_cast<ComponentID>(block_comp));
}

/*****************************************************************************/
void FrameEncoder::encodeBlockWithPrediction(
    const omnidirectional_layer::Block &b,
    omnidirectional_layer::IBlockPredictionManager::prediction_t prediction) {

  omnidirectional_layer::BlockPrediction::ePredType pred =
      omnidirectional_layer::BlockPrediction::toPredType(prediction);

  ICOV_TRACE << "Encode block " << b.getIdSelf() << " with prediction "
             << omnidirectional_layer::BlockPrediction::toString(pred)
             << " on channel " << video::channelToStr(block_comp);

  int x_block = b.parent()->col(b.u());
  int y_block = b.parent()->row(b.v());

  Position pos(static_cast<float>(x_block) /
                   static_cast<float>(m_params.ICU_Size.width),
               static_cast<float>(y_block) /
                   static_cast<float>(m_params.ICU_Size.height));

  const auto &es = this->m_enc_struc;
  uint icuIndex = es.get_icuIndex(pos);

  const auto &icu_enc = es.getICU(icuIndex);
  const auto &si_map = icu_enc.get_intraSIsMap();

  if (si_map.count(pred) > 0 ||
      (frame() > 0 &&
       pred == icov::omnidirectional_layer::BlockPrediction::ePredType::INTER))
    encode_icu(b, icu_enc, pred);
}

void icov::encoder::FrameEncoder::endEncoding() {
  this->m_icov_file->writeFrameSize();
}

} // namespace encoder
} // namespace icov
