#include "FrameEncoder.h"

#include "FastArray.h"
#include "enc_helpers.h"
#include "encoder/interface/IEncoder.h"

#include "icov_io.h"
#include "ldpc_codec_layer/BitSymbolProvider.h"
#include "ldpc_codec_layer/interface/IInteractiveCoder.h"
#include "omnidirectional_layer/Block.h"

#include "omnidirectional_layer/BlockPredictionManager.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include "video_layer/ExtractedStreamDebugger.h"
#include "video_layer/encoded_stream.h"
#include "video_layer/extracted_stream.h"
#include "video_layer/icov_stream.h"
#include "video_layer/vtm_opencv_utility.h"
#include "video_layer/vtm_utility.h"
#include <algorithm>
#include <boost/algorithm/algorithm.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <cstdint>
#include <memory>
#include <omp.h>
#include <opencv2/plot.hpp>
#include <sstream>
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
          }),
      m_init(true), block_comp(0) {

  //  xData.create(1, 100, CV_64F); // 1 Row, 100 columns, Double
  //  yData.create(1, 100, CV_64F);

  //  for (int i = 0; i < 100; ++i) {
  //    xData.at<double>(i) = i / 10.0;
  //    yData.at<double>(i) = (1.0 * i * i) / 100;
  //  }
  //  plot = cv::plot::Plot2d::create(xData, yData);
  //  plot->setPlotSize(100, 1000);
  //  plot->setMaxX(10);
  //  plot->setMinX(0);
  //  plot->setMaxY(100);
  //  plot->setMinY(-1);
  //  plot->render(display);
  //  cv::imshow("Plot", display);
}

/*****************************************************************************/
FrameEncoder::~FrameEncoder() { ICOV_TRACE << "Destroy FrameEncoder"; }

/*****************************************************************************/
void icov::encoder::FrameEncoder::serializeSyndrome(bool access_block) {
  if (access_block) {
    for (int c = 0; c < (int)m_numComp; ++c) {
      m_icov_file->setChannel(c);
      m_icov_file->startWritePrediction(
          omnidirectional_layer::BlockPrediction::ePredType::NONE);
      // store syndrome per bitplane
      Syndrome access_block_syndrome(m_channelCoder[c].access_block[0].size());
      for (const auto &s : m_channelCoder[c].access_block) {
        access_block_syndrome.fillFrom(s);
        m_icov_file->writeSyndrome(access_block_syndrome);
      }
      m_icov_file->endWritePrediction(
          omnidirectional_layer::BlockPrediction::ePredType::NONE);
    }
  }

  for (int c = 0; c < (int)m_numComp; ++c) {
    m_icov_file->setChannel(c);
    // store syndrome per bitplane
    for (const auto &s : m_channelCoder[c].bitplanes) {
      //      std::cout << "COMP " << c << " SYNDROME " << s.stored_synd.size()
      //                << std::endl;
      if (s.stored_synd.size() > 0) {
        m_icov_file->writeSyndrome(s.stored_synd);
      }
    }
  }
}

/*****************************************************************************/
void icov::encoder::FrameEncoder::serializeParameters() {

  // Loop over each component
  for (int c = 0; c < (int)m_numComp; ++c) {
    m_icov_file->setChannel(c);
    // Store syndrome per bitplane
    std::cout << "<COMP " << c << "> BP";
    m_icov_file->writeBitplaneCount(m_channelCoder[c].bitplanes.size());

    std::cout << " S";

    for (const auto &s : m_channelCoder[c].bitplanes) {
      m_icov_file->writeStoredSyndromeSize(s.stored_synd_rate, 0, 0);

      // s.stored_synd_rate.c, s.stored_synd_rate.r, s.stored_synd_rate.m
    }
    std::cout << std::endl;
  }

  for (omnidirectional_layer::IBlockPredictionManager::prediction_t p :
       bitstream::PredictionID) {
    for (int c = 0; c < (int)m_numComp; ++c) {
      m_icov_file->setChannel(c);
      m_icov_file->startWritePrediction(p);
      std::cout << "<SI "
                << omnidirectional_layer::BlockPrediction::toString(
                       omnidirectional_layer::BlockPrediction::toPredType(p))
                << " COMP " << c << "> ";
      if (m_channelCoder[c].si.count(p)) {
        std::cout << "M " << m_channelCoder[c].si[p].dirmode << " ";
        m_icov_file->writeSIMode(1 + m_channelCoder[c].si[p].dirmode);
        m_icov_file->writeQaryParams(m_channelCoder[c].si[p].stored_pz);
        if (m_channelCoder[c].si[p].stored_pz.codingflag()) {
          std::cout << "R ";
          for (int bp = 0; bp < (int)m_channelCoder[c].bitplanes.size(); ++bp) {
            if (m_channelCoder[c].bitplanes[bp].stored_numcodes.count(p)) {

              if (m_channelCoder[c].bitplanes[bp].sourceFlag) {
                m_icov_file->writeSIRate(
                    bitstream::enum_t::SyndromeType::SyndromeType_SOURCE, 1.0,
                    m_channelCoder[c].n);
              } else {
                m_icov_file->writeSIRate(
                    m_channelCoder[c].bitplanes[bp].stored_numcodes.at(p).c,
                    m_channelCoder[c].bitplanes[bp].stored_numcodes.at(p).r,
                    m_channelCoder[c].bitplanes[bp].stored_numcodes.at(p).m);
              }
            }
          }
          std::cout << std::endl;
        }
      } else if (p != omnidirectional_layer::BlockPrediction::ePredType::NONE) {
        m_icov_file->writeSIMode(bitstream::flag_t::UnavailableSI);
      }
      m_icov_file->endWritePrediction(p);
    }
  }
}

/*****************************************************************************/
void icov::encoder::FrameEncoder::serializeBlockData(
    bool access_block, const omnidirectional_layer::Block &b) {

  ICOV_DEBUG << "Serialize block " << b.getIdSelf();

  // Start writing block data
  m_icov_file->startWriteBlock(b);

  // Store bitplanes rates per SI and per channel
  serializeParameters();

  // WRITE SYNDROMES
  // ACCESS BLOCK
  serializeSyndrome(access_block);

  m_icov_file->writeBlockSize();

  // End writing block data
  m_icov_file->endWriteBlock(b);
}

/*****************************************************************************/
void FrameEncoder::encodeBlock(
    const omnidirectional_layer::Block &b, bool access_block,
    const icov::omnidirectional_layer::IBlockPredictionManager
        &block_prediction_mgr) {

  if (m_icov_file->contentParameterSet().icu_count > 0 &&
      b.getId(omnidirectional_layer::Block::SELF) >
          m_icov_file->contentParameterSet().icu_count)
    return;

  ICOV_DEBUG << "start encode block "
             << b.getId(omnidirectional_layer::Block::SELF);

  const auto v = getAvailablePredictions(b, access_block, block_prediction_mgr);

  m_channelCoder = std::vector<ChannelCoder>(m_numComp);

  for (int c = 0; c < (int)m_numComp; ++c) {
    block_comp = c;
    //#pragma omp parallel for
    for (omnidirectional_layer::IBlockPredictionManager::prediction_t p : v)
      encodeBlockWithPrediction(b, p);
  }

  serializeBlockData(access_block, b);

  //  if (debug_enabled())
  //    saveBlockDebug(access_block, b);

  ICOV_DEBUG << "end encode block "
             << b.getId(omnidirectional_layer::Block::SELF);
}

/*****************************************************************************/
void FrameEncoder::processInput(const IInputMedia<cv::Mat> &input) {
  cv::Mat img_src = input.getCurrentFrame();

  ICOV_ASSERT_MSG(img_src.total() > 0, "Cannot encode empty image");

  // CONSTANTS
  const Size icuSize_luma =
      Size(m_params.ICU_Size.width, m_params.ICU_Size.height);

  // end constants

  // resize
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

  //  PelStorage picTemp;
  //  picTemp.create(extendedFrameArea);

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

  //   END INTRA PRED
}

void icov::encoder::FrameEncoder::checkDecoding(
    ldpc_codec_layer::AffInteractiveCoder &encoder,
    const ldpc_codec_layer::BitSymbolProvider &bsp_enc,
    const icov::NumcodeIdxVec &si_numcodes, const icov::SyndromesVec &si_synd,
    ComponentID comp) {

  ldpc_codec_layer::BitSymbolProvider bsp_dec(encoder.get_n());

  bsp_dec.set_si_and_qaryDistribution(bsp_enc.si_symboles(),
                                      bsp_enc.distribution());

  for (size_t i = 0; i < si_numcodes.size(); i++) {

    ICOV_TRACE << "Check decoding bitplane " << i << std::endl;
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

  int bitplaneCount = bsp_enc.bitplane_count();
  const auto pred_id =
      static_cast<omnidirectional_layer::IBlockPredictionManager::prediction_t>(
          pred);

  ICOV_TRACE << bitplaneCount << " bitplanes to save";

  //    std::cout << "COMP " << comp << " PRED " << predID << ": "
  //              << m_channelCoder[comp].bitplanes.size() << ", " <<
  //              si_synd.size()
  //              << ", " << si_numcodes.size() << std::endl;

  m_channelCoder[comp].si[pred_id].stored_pz = bsp_enc.distribution();
  m_channelCoder[comp].si[pred_id].dirmode = mode;
  m_channelCoder[comp].n = encoder.get_n();
  // icu_enc.get_intraSIsMap().at(pred).intraDir[block_comp];

  // first SI stored (no bitplanes stored for comp)
  if (m_channelCoder[comp].bitplanes.size() == 0) {
    if (access_block)
      m_channelCoder[comp].access_block = bsp_enc.src_bits();
    m_channelCoder[comp].bitplanes = std::vector<BitplaneCoder>(
        bitplaneCount, BitplaneCoder{.minRate = 0,
                                     .stored_synd_rate = 0,
                                     .stored_synd = icov::Syndrome(),
                                     .noSourceThreshold = 0.0,
                                     .sourceFlag = false});
    for (size_t i = 0; i < si_synd.size(); ++i) {
      m_channelCoder[comp].bitplanes[i].stored_synd_rate = si_numcodes[i];
      m_channelCoder[comp].bitplanes[i].stored_synd = si_synd[i];
      m_channelCoder[comp].bitplanes[i].noSourceThreshold = 0.0;
      m_channelCoder[comp].bitplanes[i].sourceFlag = false;
      m_channelCoder[comp].bitplanes[i].minRate = si_numcodes[i];
    }
  }

  if (bsp_enc.distribution().codingflag()) {

#ifdef ICOV_ASSERTION_ENABLED
    icov::assert::check(
        (int)m_channelCoder[comp].bitplanes.size() == bitplaneCount &&
            si_synd.size() == si_numcodes.size(),
        "Bitplane count does not match between SI of same block");
#endif

    for (int i = 0; i < bitplaneCount; ++i) {
      {
        std::cout << omnidirectional_layer::BlockPrediction::toString(pred)
                  << " COMP " << comp << " BP " << i << std::endl;
        // icov::io::write_vector(std::cout, si_synd, ' ');
        std::cout << si_synd[i].to_string(" ") << std::endl;
      }

      if (si_numcodes[i] >
              bitstream::enum_t::SyndromeType::SyndromeType_SOURCE &&
          si_numcodes[i] < m_channelCoder[comp].bitplanes[i].minRate)
        m_channelCoder[comp].bitplanes[i].minRate = si_numcodes[i];

      if (si_numcodes[i] ==
          bitstream::enum_t::SyndromeType::SyndromeType_NO_CODING) {
        m_channelCoder[comp].bitplanes[i].stored_numcodes[pred_id].c =
            si_numcodes[i];
      } else if (!m_channelCoder[comp].bitplanes[i].sourceFlag) {

        m_channelCoder[comp].bitplanes[i].noSourceThreshold += encoder.get_r(
            bitstream::inverseQuantizedSize(si_numcodes[i], encoder),
            encoder.get_n());

        m_channelCoder[comp].bitplanes[i].stored_numcodes[pred_id].c =
            si_numcodes[i];

        m_channelCoder[comp].bitplanes[i].stored_numcodes[pred_id].m =
            bitstream::inverseQuantizedSize(
                m_channelCoder[comp].bitplanes[i].stored_numcodes[pred_id].c,
                encoder);

        m_channelCoder[comp].bitplanes[i].stored_numcodes[pred_id].r =
            encoder.get_r(
                m_channelCoder[comp].bitplanes[i].stored_numcodes[pred_id].m,
                encoder.get_n());

        ICOV_TRACE
            << m_channelCoder[comp].bitplanes[i].stored_numcodes[pred_id].r
            << ", "
            << m_channelCoder[comp].bitplanes[i].stored_numcodes[pred_id].m
            << ", "
            << m_channelCoder[comp].bitplanes[i].stored_numcodes[pred_id].c;

        if (m_channelCoder[comp].bitplanes[i].noSourceThreshold >
            m_params.MaxRateNoCoding) {
          m_channelCoder[comp].bitplanes[i].sourceFlag = true;
          m_channelCoder[comp].bitplanes[i].stored_synd.fillFrom(
              bsp_enc.src_bits()[i]);
          m_channelCoder[comp].bitplanes[i].stored_synd_rate =
              bitstream::enum_t::SyndromeType::SyndromeType_SOURCE;
          std::cout << "SOURCE !" << std::endl;
        } else if (si_synd[i].size() >
                   m_channelCoder[comp].bitplanes[i].stored_synd.size())
        /*STORED SYNDROME IF LONGER*/ {
          m_channelCoder[comp].bitplanes[i].stored_synd = si_synd[i];
          m_channelCoder[comp].bitplanes[i].stored_synd_rate = si_numcodes[i];
        }
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

  int bitplaneCount = bsp_enc.bitplane_count();
  const auto pred_id =
      static_cast<omnidirectional_layer::IBlockPredictionManager::prediction_t>(
          pred);

  ICOV_TRACE << "save in debug";
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

  std ::cout << "GT SYND " << comp << " = ";

  channelDbg.storedSyndromeSize.resize(m_channelCoder[comp].bitplanes.size());
  std::transform(m_channelCoder[comp].bitplanes.cbegin(),
                 m_channelCoder[comp].bitplanes.cend(),
                 channelDbg.storedSyndromeSize.begin(), [](const auto &x) {
                   std ::cout << " " << x.stored_synd_rate;
                   return x.stored_synd_rate;
                 });

  std ::cout << std::endl;

  // Checks if the block is an access block and there is no saved data
  if (b.isAccessBlock() &&
      channelDbg.si_map.count(
          omnidirectional_layer::BlockPrediction::ePredType::NONE) == 0) {

    auto &si_dbg =
        channelDbg
            .si_map[omnidirectional_layer::BlockPrediction::ePredType::NONE];

    si_dbg.mode = bitstream::flag_t::AccessBlock;
    si_dbg.pz = ldpc_codec_layer::QaryDistribution();
    si_dbg.pz.setBitplane_count(m_channelCoder[comp].access_block.size());
    ICOV_DEBUG << "ACCESS BLOCK BP COUNT "
               << m_channelCoder[comp].access_block.size();
    for (const auto &s : m_channelCoder[comp].access_block) {
      bitstream::BitplaneStream bpstr;
      bpstr.numcode = bitstream::enum_t::SyndromeType_SOURCE;
      bpstr.syndrome.fillFrom(s);
      si_dbg.bitplanes.push_back(bpstr);
    }
  }

  bitstream::VideoStreamDebugger::get_mutable_instance()
      .setDebuggingPredictionID(pred);

  // checks if there is no existing debug for this prediction and throws an
  // error if there is
  icov::assert::check(!bitstream::VideoStreamDebugger::get_const_instance()
                           .predictionAvailable(),
                      "Prediction debug already exists");

  auto &si_dbg = channelDbg.si_map[pred];

  si_dbg.values = bsp_enc.si_symboles();
  si_dbg.mode = m_channelCoder[comp].si[pred_id].dirmode;
  si_dbg.pz = m_channelCoder[comp].si[pred_id].stored_pz;

  if (si_dbg.pz.codingflag()) {
    // adds each bitplane for the given prediction to the debugging
    // bitplanes vector
    for (int i = 0; i < bitplaneCount; ++i) {
      ICOV_TRACE << "save in debug " << i;

      bitstream::BitplaneStream bpstr;
      bpstr.numcode =
          m_channelCoder[comp].bitplanes[i].stored_numcodes.at(pred_id).c;

      if (bpstr.numcode ==
          bitstream::enum_t::SyndromeType::SyndromeType_NO_CODING) {
        bpstr.syndrome = Syndrome();
      } else if (bpstr.numcode ==
                 bitstream::enum_t::SyndromeType::SyndromeType_SOURCE) {
        bpstr.syndrome = m_channelCoder[comp].bitplanes[i].stored_synd;
      } else {
        bpstr.syndrome = si_synd[i];
      }
      si_dbg.bitplanes.push_back(bpstr);
    }
  }
}

/*****************************************************************************/
void icov::encoder::FrameEncoder::encode_tu(
    const icov::omnidirectional_layer::Block &b,
    const icov::video::IcuEncoder &icu_enc,
    const icov::video::SideInformationUnit &si,
    icov::omnidirectional_layer::BlockPrediction::ePredType pred,
    ComponentID comp, int tu_index) {

  const auto pred_id =
      static_cast<omnidirectional_layer::IBlockPredictionManager::prediction_t>(
          pred);

  const auto &tu = icu_enc.get_sourceX().TUs[tu_index];
  const auto &si_tu = si.TUs[tu_index];

  auto tu_coeff = tu.getQuantizedTransformCoeffs(comp);
  size_t tu_size = tu_coeff.area();

  auto &encoder = m_pred_encoder.get_coder(pred_id, tu_size);

  // encode outputs for the current SI: vector of rates and vector of syndrome
  // (one rate and syndrome per bitplane)
  icov::NumcodeIdxVec si_numcodes;
  icov::SyndromesVec si_synd;

  ldpc_codec_layer::BitSymbolProvider bsp_enc =
      tu.instantiateBitSymbolProvider(comp, si_tu, m_params.BSP_Params);

  if (!bsp_enc.distribution().codingflag()) {
    ICOV_TRACE << "SI symbole identical... no coding...";
    std::stringstream sstr;
    sstr << "\nsrc = ";
    io::write_vector(sstr, bsp_enc.src_symboles(), ' ');
    sstr << "\n si = ";
    io::write_vector(sstr, bsp_enc.si_symboles(), ' ');
    ICOV_TRACE << sstr.str().c_str();
  } else {
    encoder.encode_bitplanes(bsp_enc, si_numcodes, si_synd);

    // CHECK DECODING
    if (m_params.CheckDecoding) {
      checkDecoding(encoder, bsp_enc, si_numcodes, si_synd, comp);
    }
    // END CHECK DECODING
  }
// encoding and decoding check done, now save syndrome if longer than current
// saved
#pragma omp critical
  {
    // Save syndrome if longer than current saved
    // void icov::encoder::FrameEncoder::saveDataToEncode(
    //    bool access_block, uint mode,
    //    ldpc_codec_layer::AffInteractiveCoder &encoder,
    //    const ldpc_codec_layer::BitSymbolProvider &bsp_enc,
    //    const icov::NumcodeIdxVec &si_numcodes, const icov::SyndromesVec
    //    &si_synd, icov::omnidirectional_layer::BlockPrediction::ePredType
    //    pred, ComponentID comp)
    saveDataToEncode(b.isAccessBlock(),
                     icu_enc.get_intraSIsMap().at(pred).intraDir[block_comp],
                     encoder, bsp_enc, si_numcodes, si_synd, pred, comp);

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
}

void icov::encoder::FrameEncoder::encode_si(
    const omnidirectional_layer::Block &b,
    const icov::video::IcuEncoder &icu_enc,
    const icov::video::SideInformationUnit &si,
    omnidirectional_layer::BlockPrediction::ePredType pred, ComponentID comp) {

  icov::assert::equal(
      (int)si.TUs.size(), 1,
      "This version of ICOV can only manage one transform unit per block");

  for (int tu_index = 0; tu_index < (int)si.TUs.size(); tu_index++) {
    encode_tu(b, icu_enc, si, pred, comp, tu_index);
  }
}

void icov::encoder::FrameEncoder::encode_icu(
    const omnidirectional_layer::Block &b,
    const icov::video::IcuEncoder &icu_enc,
    omnidirectional_layer::BlockPrediction::ePredType pred) {

  auto si = icu_enc.get_sideInformation(pred);
  icov::assert::not_null(si);
  encode_si(b, icu_enc, *si, pred, static_cast<ComponentID>(block_comp));
}

/*****************************************************************************/
void FrameEncoder::encodeBlockWithPrediction(
    const omnidirectional_layer::Block &b,
    omnidirectional_layer::IBlockPredictionManager::prediction_t prediction) {

  omnidirectional_layer::BlockPrediction::ePredType pred =
      omnidirectional_layer::BlockPrediction::toPredType(prediction);

  ICOV_DEBUG << "Encode block with pred "
             << omnidirectional_layer::BlockPrediction::toString(pred);

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

  if (si_map.count(pred) > 0)
    encode_icu(b, icu_enc, pred);
}
} // namespace encoder
} // namespace icov
