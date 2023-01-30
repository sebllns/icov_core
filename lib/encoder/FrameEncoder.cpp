#include "FrameEncoder.h"

#include "BitSymbolProvider.h"
#include "common_types.h"
#include "enc_helpers.h"
#include "encoded_stream.h"
#include "encoder/interface/IEncoder.h"
#include "helpers.h"
#include "omnidirectional_layer/BlockPrediction.h"
#include "opencv2/plot.hpp"
#include "tools/DebugTools.h"
#include "video_layer/vtm_opencv_utility.h"
#include "video_layer/vtm_utility.h"

#include <boost/algorithm/algorithm.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <cstdint>
#include <omp.h>
#include <vector>

namespace fs = boost::filesystem;
using namespace icov::encoder;
using SI_SIDE = BlockPrediction::predType;

/*****************************************************************************/
FrameEncoder::FrameEncoder(int frame, const params &p)
    : IFrameEncoder(frame), m_params(p),
      m_numComp(getNumberValidComponents(p.chFormat)),
      m_pred_encoder(p.program_args,
                     std::vector<uint16_t>{
                         BlockPrediction::RIGHT,
                         BlockPrediction::LEFT,
                         BlockPrediction::TOP,
                         BlockPrediction::BOTTOM,
                         BlockPrediction::TOP_RIGHT,
                         BlockPrediction::TOP_LEFT,
                         BlockPrediction::BOTTOM_RIGHT,
                         BlockPrediction::BOTTOM_LEFT,
                     }),
      block_comp(0) {

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
void FrameEncoder::encodeBlock(std::ostream &output, const Block &b,
                               bool access_block,
                               const IBlockPrediction &block_prediction) {

  if (b.getId(Block::SELF) > 2)
    return;

  ICOV_DEBUG << "start encode block " << b.getId(Block::SELF);

  const auto v = getAvailablePredictions(b, access_block, block_prediction);

  m_channelCoder = std::vector<ChannelCoder>(m_numComp);

  for (int c = 0; c < (int)m_numComp; ++c) {
    block_comp = c;
#pragma omp parallel for
    for (uint16_t p : v)
      encodeBlockWithPrediction(output, b, p);
  }

  std::cout << "START HEADER\n";

  for (int c = 0; c < (int)m_numComp; ++c) {
    // store syndrome per bitplane
    std::cout << "BP ";
    m_icov_file->writeSIRate(m_channelCoder[c].bitplanes.size());
    std::cout << std::endl;

    for (const auto &s : m_channelCoder[c].bitplanes) {
      std::cout << "COMP " << c << " SYNDROME ";
      m_icov_file->writeSIRate(s.stored_synd_rate);
      std::cout << std::endl;
    }

    for (const auto &s : m_channelCoder[c].bitplanes) {
#ifdef ICOV_ASSERTION_ENABLED
      icov::helpers::assert::check(s.accessBlock.size() > 0,
                                   "No access blocks bits !");
#endif
      if (access_block) {
        std::cout << "COMP " << c << " ACCESS BLOCK " << s.accessBlock.size()
                  << std::endl;
        m_icov_file->writeSyndrome(s.accessBlock);
      }
    }
  }

  std::cout << "END HEADER\n";

  for (int c = 0; c < (int)m_numComp; ++c) {
    // store syndrome per bitplane
    for (const auto &s : m_channelCoder[c].bitplanes) {
      std::cout << "COMP " << c << " SYNDROME " << s.stored_synd.size()
                << std::endl;
      if (s.stored_synd.size() > 0) {
        m_icov_file->writeSyndrome(s.stored_synd);
      }
    }
  }

  // store bitplanes rates per SI and per channel
  for (uint16_t p : PredictionID) {
    for (int c = 0; c < (int)m_numComp; ++c) {
      if (m_channelCoder[c].si.count(p)) {
        m_icov_file->writeSIMode(m_channelCoder[c].si[p].dirmode);
        m_icov_file->writeQaryParams(m_channelCoder[c].si[p].stored_pz);
        if (m_channelCoder[c].si[p].stored_pz.lx > 0) {
          std::cout << "WRITE RATES SI " << p << " COMP " << c;
          for (int bp = 0; bp < (int)m_channelCoder[c].bitplanes.size(); ++bp) {
            if (m_channelCoder[c].bitplanes[bp].stored_numcodes.count(p)) {
              m_icov_file->writeSIRate(
                  m_channelCoder[c].bitplanes[bp].sourceFlag
                      ? 1
                      : m_channelCoder[c].bitplanes[bp].stored_numcodes.at(p));
            }
          }
          std::cout << std::endl;
        }
      } else {
        m_icov_file->writeQaryParams(BitSymbolProvider::Pz(0, 0, 0, 0));
      }
    }
  }
  m_icov_file->writeBlockSize();

  ICOV_DEBUG << "end encode block " << b.getId(Block::SELF);
}

/*****************************************************************************/
void FrameEncoder::processInput(std::ostream &output,
                                const IInputMedia<cv::Mat> &input) {

  cv::Mat img_src = input.getCurrentFrame();

  ICOV_ASSERT_MSG(img_src.total() > 0, "Cannot encode empty image");

  // CONSTANTS
  const Size icuSize_luma =
      Size(m_params.icuSize_luma.width, m_params.icuSize_luma.height);
  const Size tuSize_luma =
      Size(m_params.tuSize_luma.width, m_params.tuSize_luma.height);

  // end constants

  // resize
  const cv::Size size_before = img_src.size();
  resize_covering_blocks(img_src,
                         cv::Size(icuSize_luma.width, icuSize_luma.height));
  const cv::Size size_after = img_src.size();

  if (size_after != size_before) {
    ICOV_LOG(warning) << "Image resized from " << size_before << " to "
                      << size_after;

    ICOV_LOG(warning) << "Aspect changed from "
                      << double(size_before.width) / double(size_before.height)
                      << " to "
                      << double(size_after.width) / double(size_after.height);
  }

  const int sourceWidth = img_src.cols;
  const int sourceHeight = img_src.rows;
  // end resize

  const UnitArea extendedFrameArea(
      m_params.chFormat,
      Area(0, 0, sourceWidth + 2 * m_params.margin_x_360luma, sourceHeight));
  const UnitArea mainFrameArea(
      m_params.chFormat,
      Area(m_params.margin_x_360luma, 0, sourceWidth, sourceHeight));

  //  PelStorage picTemp;
  //  picTemp.create(extendedFrameArea);

  m_vvc_bufs.create(PIC_ORIGINAL, extendedFrameArea);
  m_vvc_bufs.create(PIC_RECONSTRUCTION, extendedFrameArea);
  m_vvc_bufs.create(PIC_PREDICTION, extendedFrameArea);

  std::vector<BlockPrediction::predType> predVec = {
      BlockPrediction::RIGHT,        BlockPrediction::LEFT,
      BlockPrediction::TOP,          BlockPrediction::BOTTOM,
      BlockPrediction::TOP_LEFT,     // 0b00101
      BlockPrediction::BOTTOM_LEFT,  // 0b01001
      BlockPrediction::BOTTOM_RIGHT, // 0b01010
      BlockPrediction::TOP_RIGHT};

  for (auto pk : predVec) {
    m_vvc_bufs.create_link(PIC_PREDICTION + pk, PIC_PREDICTION);
  }

  PelUnitBuf mainFrameOrig = m_vvc_bufs.get(PIC_ORIGINAL).getBuf(mainFrameArea);

  ICOV_ASSERT_MSG((int)(mainFrameOrig.get(COMPONENT_Y).width) == img_src.cols,
                  "Width does not match");

  ICOV_ASSERT_MSG((int)(mainFrameOrig.get(COMPONENT_Y).height) == img_src.rows,
                  "Height does not match");

  /// TODO : MOVE INSIDE SPECIFIC FUNCTION
  VTM_OPENCV_Utility::cvMatToYUV(img_src, mainFrameOrig);
  m_enc_struc
      .create360VideoInteractiveCodingUnits_withoutAccessBlockComputation(
          m_params.chFormat, sourceWidth, sourceHeight,
          m_params.margin_x_360luma, icuSize_luma.width, icuSize_luma.height,
          tuSize_luma.width, tuSize_luma.height);

  VTM_Utility::quantizeFrame(
      m_params.qp, m_vvc_bufs.get(PIC_ORIGINAL),
      m_vvc_bufs.get(PIC_RECONSTRUCTION), m_enc_struc,
      m_params.internalBitDepth,
      m_params.maxLog2TrDynamicRange); // give the whole extended image (
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
        m_vvc_bufs, compID, m_params.internalBitDepth,
        m_params.tuSizeAfterRegrouping_luma.width,
        m_params.tuSizeAfterRegrouping_luma.height, false, m_params.qp,
        m_params.maxLog2TrDynamicRange);
  }
  ICOV_DEBUG << "Successfully computed intra predictions";

  //   END INTRA PRED
}

/*****************************************************************************/
void icov::encoder::FrameEncoder::encode_tu(
    std::ostream &output, const Block &b,
    const icov::video::IcuEncoder &icu_enc,
    const icov::video::SideInformationUnit &si, BlockPrediction::predType pred,
    ComponentID comp, int tu_index) {

  const auto predID = static_cast<uint16_t>(pred);

  const auto &x_tu = icu_enc.get_sourceX().TUs[tu_index];
  const auto &si_tu = si.TUs[tu_index];

  auto x_tuCoeff = x_tu.getQuantizedTransformCoeffs(comp);

  size_t tu_size = x_tuCoeff.area();

  // encode outputs for the current SI: vector of rates and vector of syndrome
  // (one rate and syndrome per bitplane)
  icov::NumcodeIdxVec si_numcodes;
  icov::SyndromesVec si_synd;

  BitSymbolProvider bsp =
      x_tu.getBitSymbolProvider(comp, si_tu, m_params.bsp_params);

  auto &encoder = m_pred_encoder.get_coder(predID, tu_size);

  if (bsp.q0() == 1) {
    ICOV_TRACE << "SI identical... no coding...";
  } else {
    encoder.encode_bitplanes(bsp, si_numcodes, si_synd);

    // CHECK DECODING
    if (m_params.check_decoding) {

      BitSymbolProvider bsp_dec(tu_size);

      ICOV_TRACE << bsp.si_symboles().size();
      ICOV_TRACE << bsp.z_min();
      ICOV_TRACE << bsp.z_max();
      ICOV_TRACE << bsp.q0();
      ICOV_TRACE << bsp.x_binary_length();

      bsp_dec.set_si(bsp.si_symboles(), bsp.z_min(), bsp.z_max(), bsp.q0(),
                     bsp.x_binary_length());

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
        bsp_dec.set_si(bsp.si_symboles(), bsp.z_min(), bsp.z_max(), bsp.q0(),
                       bsp.x_binary_length());
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
    std::cout << "COMP " << comp << " PRED " << predID << ": "
              << m_channelCoder[comp].bitplanes.size() << ", " << si_synd.size()
              << ", " << si_numcodes.size() << std::endl;

    m_channelCoder[comp].si[predID].stored_pz = bsp.getPz();
    m_channelCoder[comp].si[predID].dirmode =
        icu_enc.get_intraSIsMap().at(pred).intraDir[block_comp];

    // first SI stored (no bitplanes stored for comp)
    if (m_channelCoder[comp].bitplanes.size() == 0) {
      m_channelCoder[comp].bitplanes =
          std::vector<BitplaneCoder>(si_synd.size());
      for (size_t i = 0; i < m_channelCoder[comp].bitplanes.size(); ++i) {
        m_channelCoder[comp].bitplanes[i].stored_synd_rate = si_numcodes[i];
        m_channelCoder[comp].bitplanes[i].stored_synd = si_synd[i];
        m_channelCoder[comp].bitplanes[i].noSourceThreshold = 0.0;
        m_channelCoder[comp].bitplanes[i].sourceFlag = false;
        m_channelCoder[comp].bitplanes[i].minRate = si_numcodes[i];
        if (b.isAccessBlock()) {
          m_channelCoder[comp].bitplanes[i].accessBlock.fillFrom(
              bsp.src_bits()[i]);
        }
      }
    }

    if (bsp.q0() < 1) {

#ifdef ICOV_ASSERTION_ENABLED
      icov::helpers::assert::check(
          m_channelCoder[comp].bitplanes.size() == si_synd.size() &&
              m_channelCoder[comp].bitplanes.size() == si_numcodes.size(),
          "Bitplane count does not match between SI of same block");
#endif

      for (size_t i = 0; i < m_channelCoder[comp].bitplanes.size(); ++i) {

        if (si_numcodes[i] > 0 &&
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
              m_params.maxRateNoCoding) {
            m_channelCoder[comp].bitplanes[i].sourceFlag = true;
            m_channelCoder[comp].bitplanes[i].stored_synd.fillFrom(
                bsp.src_bits()[i]);
            m_channelCoder[comp].bitplanes[i].stored_synd_rate = 1;
            std::cout << "SOURCE !" << std::endl;
          } else if (si_synd[i].size() >
                     m_channelCoder[comp].bitplanes[i].stored_synd.size()) {
            m_channelCoder[comp].bitplanes[i].stored_synd = si_synd[i];
            m_channelCoder[comp].bitplanes[i].minRate = si_numcodes[i];
          }
        }
      }
    }
  }
}

const std::shared_ptr<OutputIcov> &FrameEncoder::icovFile() const {
  return m_icov_file;
}

void FrameEncoder::setIcovFile(
    const std::shared_ptr<OutputIcov> &newIcov_file) {
  m_icov_file = newIcov_file;
}

void icov::encoder::FrameEncoder::encode_si(
    std::ostream &output, const Block &b,
    const icov::video::IcuEncoder &icu_enc,
    const icov::video::SideInformationUnit &si, BlockPrediction::predType pred,
    ComponentID comp) {

  icov::helpers::assert::equal(
      (int)si.TUs.size(), 1,
      "This version of ICOV can only manage one transform unit per block");

  for (int tu_index = 0; tu_index < (int)si.TUs.size(); tu_index++) {
    encode_tu(output, b, icu_enc, si, pred, comp, tu_index);
  }
}

void icov::encoder::FrameEncoder::encode_icu(
    std::ostream &output, const Block &b,
    const icov::video::IcuEncoder &icu_enc, BlockPrediction::predType pred) {

  auto si = icu_enc.get_sideInformation(pred);
  icov::helpers::assert::not_null(si);
  encode_si(output, b, icu_enc, *si, pred,
            static_cast<ComponentID>(block_comp));
}

/*****************************************************************************/
void FrameEncoder::encodeBlockWithPrediction(std::ostream &output,
                                             const Block &b,
                                             uint16_t prediction) {

  BlockPrediction::predType pred = BlockPrediction::toPredType(prediction);

  int x_block = b.parent()->col(b.u());
  int y_block = b.parent()->row(b.v());

  Position pos(static_cast<float>(x_block) /
                   static_cast<float>(m_params.icuSize_luma.width),
               static_cast<float>(y_block) /
                   static_cast<float>(m_params.icuSize_luma.height));

  const auto &es = this->m_enc_struc;
  uint icuIndex = es.get_icuIndex(pos);

  const auto &icu_enc = es.getICU(icuIndex);
  const auto &si_map = icu_enc.get_intraSIsMap();

  if (si_map.count(pred) > 0)
    encode_icu(output, b, icu_enc, pred);
}
