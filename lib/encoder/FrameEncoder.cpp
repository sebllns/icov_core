#include "FrameEncoder.h"

#include "common_types.h"
#include "enc_helpers.h"
#include "encoder/interface/IEncoder.h"
#include "helpers.h"
#include "ldpc_codec_layer/BitSymbolProvider.h"
#include "omnidirectional_layer/Block.h"

#include "omnidirectional_layer/BlockPredictionManager.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include "tools/DebugTools.h"
#include "video_layer/ExtractedStreamDebugger.h"
#include "video_layer/encoded_stream.h"
#include "video_layer/icov_stream.h"
#include "video_layer/vtm_opencv_utility.h"
#include "video_layer/vtm_utility.h"
#include <boost/algorithm/algorithm.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <cstdint>
#include <memory>
#include <omp.h>
#include <opencv2/plot.hpp>
#include <vector>

namespace icov {
namespace encoder {
namespace fs = boost::filesystem;
using SI_SIDE = icov::omnidirectional_layer::BlockPrediction::predType;

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
void FrameEncoder::encodeBlock(
    const omnidirectional_layer::Block &b, bool access_block,
    const icov::omnidirectional_layer::IBlockPredictionManager
        &block_prediction_mgr) {

  if (b.getId(omnidirectional_layer::Block::SELF) > 2)
    return;

  ICOV_DEBUG << "start encode block "
             << b.getId(omnidirectional_layer::Block::SELF);

  const auto v = getAvailablePredictions(b, access_block, block_prediction_mgr);

  m_channelCoder = std::vector<ChannelCoder>(m_numComp);

  for (int c = 0; c < (int)m_numComp; ++c) {
    block_comp = c;
#pragma omp parallel for
    for (omnidirectional_layer::IBlockPredictionManager::prediction_t p : v)
      encodeBlockWithPrediction(b, p);
  }

  m_icov_file->startWriteBlock(b);

  for (int c = 0; c < (int)m_numComp; ++c) {
    // store syndrome per bitplane
    std::cout << "<COMP " << c << "> BP";
    m_icov_file->writeSIRate(m_channelCoder[c].bitplanes.size());

    std::cout << " S";

    for (const auto &s : m_channelCoder[c].bitplanes) {
      m_icov_file->writeSIRate(s.stored_synd_rate);
    }
    std::cout << std::endl;
  }

  // store bitplanes rates per SI and per channel
  for (omnidirectional_layer::IBlockPredictionManager::prediction_t p :
       bitstream::PredictionID) {
    m_icov_file->startWritePrediction(p);
    for (int c = 0; c < (int)m_numComp; ++c) {
      std::cout << "<SI "
                << omnidirectional_layer::BlockPrediction::toString(
                       omnidirectional_layer::BlockPrediction::toPredType(p))
                << " COMP " << c << "> ";
      if (m_channelCoder[c].si.count(p)) {
        std::cout << "M " << m_channelCoder[c].si[p].dirmode << " ";
        m_icov_file->writeSIMode(1 + m_channelCoder[c].si[p].dirmode);
        m_icov_file->writeQaryParams(m_channelCoder[c].si[p].stored_pz);
        if (m_channelCoder[c].si[p].stored_pz.getZAmplitude() > 0) {
          std::cout << "R ";
          for (int bp = 0; bp < (int)m_channelCoder[c].bitplanes.size(); ++bp) {
            if (m_channelCoder[c].bitplanes[bp].stored_numcodes.count(p)) {
              m_icov_file->writeSIRate(
                  m_channelCoder[c].bitplanes[bp].sourceFlag
                      ? bitstream::enum_t::SyndromeType::SOURCE
                      : m_channelCoder[c].bitplanes[bp].stored_numcodes.at(p));
            }
          }
          std::cout << std::endl;
        }
      } else if (p != omnidirectional_layer::BlockPrediction::predType::NONE) {
        m_icov_file->writeSIMode(0);
      }
    }
    m_icov_file->endWritePrediction(p);
  }

  // WRITE SYNDROMES
  // ACCESS BLOCK
  if (access_block) {
    for (int c = 0; c < (int)m_numComp; ++c) {
      // store syndrome per bitplane
      Syndrome access_block_syndrome(m_channelCoder[c].access_block[0].size());
      for (const auto &s : m_channelCoder[c].access_block) {
        access_block_syndrome.fillFrom(s);
        m_icov_file->writeSyndrome(access_block_syndrome);

        if (debug_enabled()) {
        }
      }
    }
  }

  for (int c = 0; c < (int)m_numComp; ++c) {
    // store syndrome per bitplane
    for (const auto &s : m_channelCoder[c].bitplanes) {
      //      std::cout << "COMP " << c << " SYNDROME " << s.stored_synd.size()
      //                << std::endl;
      if (s.stored_synd.size() > 0) {
        m_icov_file->writeSyndrome(s.stored_synd);
      }
    }
  }

  m_icov_file->writeBlockSize();

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

    std::vector<omnidirectional_layer::BlockPrediction::predType> predVec = {
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

/*****************************************************************************/
void icov::encoder::FrameEncoder::encode_tu(
    const icov::omnidirectional_layer::Block &b,
    const icov::video::IcuEncoder &icu_enc,
    const icov::video::SideInformationUnit &si,
    icov::omnidirectional_layer::BlockPrediction::predType pred,
    ComponentID comp, int tu_index) {

  const auto predID =
      static_cast<omnidirectional_layer::IBlockPredictionManager::prediction_t>(
          pred);

  const auto &x_tu = icu_enc.get_sourceX().TUs[tu_index];
  const auto &si_tu = si.TUs[tu_index];

  auto x_tuCoeff = x_tu.getQuantizedTransformCoeffs(comp);

  size_t tu_size = x_tuCoeff.area();

  // encode outputs for the current SI: vector of rates and vector of syndrome
  // (one rate and syndrome per bitplane)
  icov::NumcodeIdxVec si_numcodes;
  icov::SyndromesVec si_synd;

  ldpc_codec_layer::BitSymbolProvider bsp =
      x_tu.getBitSymbolProvider(comp, si_tu, m_params.BSP_Params);

  auto &encoder = m_pred_encoder.get_coder(predID, tu_size);

  if (bsp.pz_params().getZAmplitude() == 0) {
    ICOV_TRACE << "SI symbole identical... no coding...";
  } else {
    encoder.encode_bitplanes(bsp, si_numcodes, si_synd);

    // CHECK DECODING
    if (m_params.CheckDecoding) {

      ldpc_codec_layer::BitSymbolProvider bsp_dec(tu_size);

      ICOV_TRACE << bsp.si_symboles().size();
      ICOV_TRACE << bsp.pz_params().getZmin();
      ICOV_TRACE << bsp.pz_params().getZmax();
      ICOV_TRACE << bsp.pz_params().getQ0();
      ICOV_TRACE << bsp.bitplane_count();

      bsp_dec.set_si(bsp.si_symboles(), bsp.pz_params());

      for (size_t i = 0; i < si_numcodes.size(); i++) {

        ICOV_TRACE << "Check decoding bitplane " << i << std::endl;
        BitVec decoded;
        try {
          ICOV_TRACE << "Check bitplane " << i;
          encoder.decode_bitplane(decoded, i, bsp_dec, si_numcodes[i],
                                  si_synd[i]);
          icov::helpers::assert::check(decoded == bsp.src_bits()[i],
                                       "Decoding BP check failed " +
                                           std::to_string(i));
        } catch (const std::exception &e) {
          ICOV_ERROR << e.what();
          break;
        }
      }

      try {
        bsp_dec.set_si(bsp.si_symboles(), bsp.pz_params());
        encoder.decode_symboles(bsp_dec, si_numcodes, si_synd);

        icov::helpers::assert::check(
            bsp_dec.src_symboles() == bsp.src_symboles(),
            "Decoding symbole failed for channel " + std::to_string(comp));
      } catch (const std::exception &e) {
        ICOV_ERROR << e.what();
      }
    }
    // END CHECK DECODING
  }
// encoding and decoding check done, now save syndrome if longer than current
// saved
#pragma omp critical
  {
    int bitplaneCount = bsp.bitplane_count();
    //    std::cout << "COMP " << comp << " PRED " << predID << ": "
    //              << m_channelCoder[comp].bitplanes.size() << ", " <<
    //              si_synd.size()
    //              << ", " << si_numcodes.size() << std::endl;

    m_channelCoder[comp].si[predID].stored_pz = bsp.pz_params();
    m_channelCoder[comp].si[predID].dirmode =
        icu_enc.get_intraSIsMap().at(pred).intraDir[block_comp];

    // first SI stored (no bitplanes stored for comp)
    if (m_channelCoder[comp].bitplanes.size() == 0) {
      if (b.isAccessBlock())
        m_channelCoder[comp].access_block = bsp.src_bits();
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

    if (bsp.pz_params().getZAmplitude() > 0) {

#ifdef ICOV_ASSERTION_ENABLED
      icov::helpers::assert::check(
          m_channelCoder[comp].bitplanes.size() == bitplaneCount &&
              si_synd.size() == si_numcodes.size(),
          "Bitplane count does not match between SI of same block");
#endif

      for (int i = 0; i < bitplaneCount; ++i) {

        {
          std::cout << omnidirectional_layer::BlockPrediction::toString(pred)
                    << " COMP " << comp << " BP " << i << std::endl;
          // icov::helpers::io::write_vector(std::cout, si_synd, ' ');
          std::cout << si_synd[i].to_string(" ") << std::endl;
        }

        if (si_numcodes[i] > 1 &&
            si_numcodes[i] < m_channelCoder[comp].bitplanes[i].minRate)
          m_channelCoder[comp].bitplanes[i].minRate = si_numcodes[i];

        if (si_numcodes[i] == 0) {
          m_channelCoder[comp].bitplanes[i].stored_numcodes[predID] = 0;
        } else if (!m_channelCoder[comp].bitplanes[i].sourceFlag) {

          m_channelCoder[comp].bitplanes[i].noSourceThreshold +=
              encoder.get_r(si_numcodes[i] - 2);

          m_channelCoder[comp].bitplanes[i].stored_numcodes[predID] =
              si_numcodes[i];

          if (m_channelCoder[comp].bitplanes[i].noSourceThreshold >
              m_params.MaxRateNoCoding) {
            m_channelCoder[comp].bitplanes[i].sourceFlag = true;
            m_channelCoder[comp].bitplanes[i].stored_synd.fillFrom(
                bsp.src_bits()[i]);
            m_channelCoder[comp].bitplanes[i].stored_synd_rate = 1;
            std::cout << "SOURCE !" << std::endl;
          } else if (si_synd[i].size() >
                     m_channelCoder[comp].bitplanes[i].stored_synd.size()) {
            m_channelCoder[comp].bitplanes[i].stored_synd = si_synd[i];
            m_channelCoder[comp].bitplanes[i].stored_synd_rate = si_numcodes[i];
          }
        }
      }
    }

    // HERE SAVE INFOS FOR DEBUG
    //    if (frame() <
    //    bitstream::VideoEncodingStreamCache::get_const_instance()
    //                      .getFrameCount()) {
    //      auto &frameDbg =
    //          bitstream::VideoEncodingStreamCache::get_mutable_instance().getFrame(
    //              frame());
    //      Block::id_t blockID = b.getIdSelf();
    //      if (blockID > frameDbg.size()) {
    //        frameDbg.resize((size_t)(blockID),
    //                        bitstream::VideoEncodingStreamCache::
    //                            MultiChannelBlockStreamDebugger(m_numComp));
    //      }
    //      auto &blockDbg = frameDbg[blockID - 1];

    //      auto &chDbg = blockDbg[comp];

    //      if (b.isAccessBlock() &&
    //      !chDbg.count(BlockPrediction::predType::NONE)) {
    //        chDbg.insert_or_assign(BlockPrediction::predType::NONE,
    //                               bitstream::SIStream());
    //        auto &predDbg = chDbg[pred];
    //        predDbg.mode = 0;
    //        predDbg.pz = BitSymbolProvider::PzParams();
    //        predDbg.bitplanes.resize(bitplaneCount);

    //        std::cout << "\n\n\n\nDBG >>>>>>>>> ACCESS BLOCK " << blockID << "
    //        "
    //                  << comp << " " << predDbg.bitplanes.size() << std::endl;

    //        for (int i = 0; i < bitplaneCount; ++i) {
    //          // predDbg.bitplanes[i].syndrome =
    //          m_channelCoder[comp].access_block[i];
    //          predDbg.bitplanes[i].numcode =
    //              bitstream::enum_t::SyndromeType::SOURCE;
    //        }
    //      }

    //      if (!chDbg.count(pred)) {
    //        chDbg.insert_or_assign(pred, bitstream::SIStream());
    //      }
    //      auto &predDbg = chDbg[pred];
    //      predDbg.mode = m_channelCoder[comp].si[predID].dirmode;
    //      predDbg.pz = m_channelCoder[comp].si[predID].stored_pz;
    //      icov::helpers::assert::equal(
    //          si_synd.size(), si_numcodes.size(),
    //          "Numcode vector and syndrome vector have same length");
    //      predDbg.bitplanes.resize(bitplaneCount);
    //      for (int i = 0; i < bitplaneCount; ++i) {
    //        predDbg.bitplanes[i].syndrome = si_synd[i];
    //        predDbg.bitplanes[i].numcode = si_numcodes[i];
    //      }
    //    }
    /* save for debug*/
  }
  /*#pragma omp critical*/
}

bool FrameEncoder::debug_enabled() const {
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
    omnidirectional_layer::BlockPrediction::predType pred, ComponentID comp) {

  icov::helpers::assert::equal(
      (int)si.TUs.size(), 1,
      "This version of ICOV can only manage one transform unit per block");

  for (int tu_index = 0; tu_index < (int)si.TUs.size(); tu_index++) {
    encode_tu(b, icu_enc, si, pred, comp, tu_index);
  }
}

void icov::encoder::FrameEncoder::encode_icu(
    const omnidirectional_layer::Block &b,
    const icov::video::IcuEncoder &icu_enc,
    omnidirectional_layer::BlockPrediction::predType pred) {

  auto si = icu_enc.get_sideInformation(pred);
  icov::helpers::assert::not_null(si);
  encode_si(b, icu_enc, *si, pred, static_cast<ComponentID>(block_comp));
}

/*****************************************************************************/
void FrameEncoder::encodeBlockWithPrediction(
    const omnidirectional_layer::Block &b,
    omnidirectional_layer::IBlockPredictionManager::prediction_t prediction) {

  omnidirectional_layer::BlockPrediction::predType pred =
      omnidirectional_layer::BlockPrediction::toPredType(prediction);

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
