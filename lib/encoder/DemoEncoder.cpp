#include "DemoEncoder.h"

#include "enc_helpers.h"
#include "helpers.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "omnidirectional_layer/BlockPrediction.h"
#include "video_layer/ICOV_Quant.h"
#include "video_layer/ICOV_Transform2d.h"
#include "video_layer/vtm_opencv_utility.h"
#include "video_layer/vtm_utility.h"

#include <boost/algorithm/algorithm.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

#define WIN_ORIG "Original"
#define WIN_RECO "Reconstruction"
#define WIN_PRED "Predictions"
#define WIN_DEBUG_DECODE "Decoded"

namespace fs = boost::filesystem;
using namespace icov::encoder;
using SI_SIDE = BlockPrediction::predType;
const int DemoEncoder::PredViewID = PIC_PREDICTION + BlockPrediction::MAX;
const int DemoEncoder::RecoViewID = DemoEncoder::PredViewID + 1;
const int DemoEncoder::DecodedDebugViewID = DemoEncoder::RecoViewID + 1;

/*****************************************************************************/
void copyBordersFor360(PelUnitBuf &mainFrame_roi, const ComponentID compID) {
  const auto &compBuf = mainFrame_roi.get(compID);

  Pel *src1 = compBuf.buf;
  Pel *dst1 = compBuf.buf + compBuf.width;

  Pel *src2 = compBuf.buf + compBuf.width - 1;
  Pel *dst2 = compBuf.buf - 1;

  for (UInt y = 0; y < compBuf.height; y++, src1 += compBuf.stride,
            dst1 += compBuf.stride, src2 += compBuf.stride,
            dst2 += compBuf.stride) {
    *dst1 = *src1;
    *dst2 = *src2;
  }
}

/*****************************************************************************/
void callBackFuncViewPred(int event, int x, int y, int flags, void *userdata) {
  static_cast<DemoEncoder *>(userdata)->onMouseViewPred(event, x, y, flags);
}

/*****************************************************************************/
void callBackFuncEncoding(int event, int x, int y, int flags, void *userdata) {
  static_cast<DemoEncoder *>(userdata)->onMouseEncode(event, x, y, flags);
}

/*****************************************************************************/
DemoEncoder::DemoEncoder(int frame, const params &p)
    : IFrameEncoder(frame), m_params(p),
      m_numComp(getNumberValidComponents(p.chFormat)),
      m_firstDisplayBlocks(true), m_pic_ptr(nullptr) {
  std::unique_ptr<AffInteractiveCoder> ycoder, uvcoder;
  icov::CoderFactory::create_coder_yuv(p.program_args, ycoder, uvcoder);

  m_coders[ycoder->get_n()] = std::move(ycoder);
  m_coders[uvcoder->get_n()] = std::move(uvcoder);
}

/*****************************************************************************/
DemoEncoder::~DemoEncoder() { std::cout << "destroy DemoEncoder" << std::endl; }

/*****************************************************************************/
void DemoEncoder::onMouseViewPred(int event, int x, int y, int flags) {
  if (event == cv::EVENT_LBUTTONDOWN) {
    int bx = x / m_params.icuSize_luma.width;
    int by = y / m_params.icuSize_luma.height;
    if (bx < (int)m_enc_struc.n_icusPerRow &&
        by < (int)m_enc_struc.n_icusPerCol)
      onSelectBlock(bx * m_params.icuSize_luma.width,
                    by * m_params.icuSize_luma.height);
  }
}

/*****************************************************************************/
void DemoEncoder::onMouseEncode(int event, int x, int y, int flags) {
  if (event == cv::EVENT_LBUTTONDOWN) {
    int bx = x / m_params.icuSize_luma.width;
    int by = y / m_params.icuSize_luma.height;
    if (bx >= 0 && by >= 0 && bx < 3 && by < 3)
      onEncodeBlock(bx, by);
  }
}

/*****************************************************************************/
// TOP LEFT = 0, 0
// BOTTOM RIGHT = 2, 2
void DemoEncoder::onEncodeBlock(int x, int y) {
  // select prediction from x, y

  std::cout << x << ", " << y << std::endl;

  uint predid = BlockPrediction::NONE;

  if (x == 0)
    predid |= BlockPrediction::LEFT;

  if (x == 2)
    predid |= BlockPrediction::RIGHT;

  if (y == 0)
    predid |= BlockPrediction::TOP;

  if (y == 2)
    predid |= BlockPrediction::BOTTOM;

  BlockPrediction::predType pred = BlockPrediction::toPredType(predid);

  std::cout << BlockPrediction::toString(pred) << std::endl;

  // encode with prediction
  auto b = m_pic_ptr->getBlock(
      m_currentBlockPosition.y / m_params.icuSize_luma.height,
      m_currentBlockPosition.x / m_params.icuSize_luma.width);

  std::cout << b->getId(Block::SELF) << std::endl;

  if (m_pic_ptr->isValidBlock(b))
    encodeBlockWithPrediction(std::cout, *b, predid);
}

/*****************************************************************************/
void DemoEncoder::onSelectBlock(int x, int y) {
  cv::Point bp = cv::Point(x, y);

  if (bp != m_currentBlockPosition) {

    auto &m_cvMat_reconstructedEquiImage = m_cvmat[RecoViewID];
    auto &m_cvMat_blockSI = m_cvmat[PredViewID];

    if (!m_firstDisplayBlocks) {
      m_cvMat_blockSI(cv::Rect(m_params.icuSize_luma.width + 1,
                               m_params.icuSize_luma.height + 1,
                               m_params.icuSize_luma.width - 1,
                               m_params.icuSize_luma.height - 1))
          .copyTo(m_cvMat_reconstructedEquiImage(
              cv::Rect(m_currentBlockPosition + cv::Point(1, 1),
                       m_params.icuSize_luma - cv::Size(1, 1))));
    }

    m_cvMat_blockSI.setTo(0);

    int x_block = x, y_block = y;

    const auto &es = this->m_enc_struc;
    uint icuIndex =
        es.get_icuIndex(Position(x_block / m_params.icuSize_luma.width,
                                 y_block / m_params.icuSize_luma.height));
    const auto &icu_enc = es.getICU(icuIndex);
    const auto &si_map = icu_enc.get_intraSIsMap();

    cv::Rect rect_src(x_block, y_block, m_params.icuSize_luma.width,
                      m_params.icuSize_luma.height);
    cv::Rect rect_dst(rect_src.width, rect_src.height, rect_src.width,
                      rect_src.height);

    cv::Mat src_block = m_cvmat[PIC_RECONSTRUCTION](rect_src);

    src_block.copyTo(m_cvMat_blockSI(rect_dst));

    std::stringstream sepstr;
    sepstr << std::string(16, '-') << "|" << std::string(10, '-');
    for (uint i = 0; i < m_numComp; i++) {
      sepstr << "|" << std::string(9, '-');
    }
    sepstr << "--";

    std::string innersep = sepstr.str();
    std::string endsep = std::string(innersep.length(), '-');

    if (DebugTools::is_release_build)
      icov::helpers::io::cls();

    // table header
    // 2 spaces are required for alignment
    std::cout << boost::format("%_15s   %-8s ") % "SI ID" % "PSNR";
    for (uint i = 0; i < m_numComp; i++) {
      std::cout << boost::format("  DIR_%-4d") % i;
    }

    std::cout << std::endl;
    // end header

    for (const auto &si : si_map) {
      std::cout << innersep << std::endl;
      cv::Mat si_block = m_cvmat[PIC_PREDICTION + si.first](rect_src);

      int bi = 0, bj = 0;

      std::stringstream predStr;

      if (si.first & BlockPrediction::TOP) {
        predStr << "TOP ";
        bi = -1;
      }
      if (si.first & BlockPrediction::BOTTOM) {
        predStr << "BOTTOM ";
        bi = 1;
      }
      if (si.first & BlockPrediction::LEFT) {
        predStr << "LEFT ";
        bj = -1;
      }
      if (si.first & BlockPrediction::RIGHT) {
        predStr << "RIGHT ";
        bj = 1;
      }

      int ybi = bi * m_params.icuSize_luma.height;
      int xbj = bj * m_params.icuSize_luma.width;

      double psnr = VTM_OPENCV_Utility::psnr(src_block, si_block);
      std::cout << boost::format("%_15s | %-8.2f ") % predStr.str() % psnr;

      for (uint c = 0; c < m_numComp; c++) {
        uint dirmode = static_cast<uint>(si.second.intraDir[c]);

        std::string dirstr;

        switch (dirmode) {
        case PLANAR_IDX:
          dirstr = std::string("PLANAR");
          break;
        case DC_IDX:
          dirstr = std::string("DC");
          break;
        case HOR_IDX:
          dirstr = std::string("HOR");
          break;
        case DIA_IDX:
          dirstr = std::string("DIA");
          break;
        case VER_IDX:
          dirstr = std::string("VER");
          break;
        case VDIA_IDX:
          dirstr = std::string("VDIA");
          break;
        case NOMODE_IDX:
          dirstr = std::string("NOMODE");
          break;
        default:
          dirstr = std::to_string(dirmode);
          break;
        }
        std::cout << boost::format("| %-8s") % dirstr;
      }

      std::cout << std::endl;

      rect_dst = cv::Rect(xbj + rect_src.width, ybi + rect_src.height,
                          rect_src.width, rect_src.height);

      si_block.copyTo(m_cvMat_blockSI(rect_dst));
    }

    std::cout << endsep << std::endl << std::endl;

    m_cvMat_reconstructedEquiImage(cv::Rect(bp, m_params.icuSize_luma))
        .setTo(cv::Scalar(0, 255, 0));

    cv::imshow(WIN_RECO, m_cvMat_reconstructedEquiImage);
    cv::imshow(WIN_PRED, m_cvMat_blockSI);

    m_currentBlockPosition = bp;

    if (m_params.enable_block_encoding) {
      std::cout << "Click on any SI in " << WIN_PRED
                << " window to encode block with the selected SI..."
                << std::endl;
    }
  }
}

/*****************************************************************************/
void DemoEncoder::processInput(std::ostream &output,
                               const IInputMedia<cv::Mat> &input) {

  std::cout << input.getPath() << std::endl;
  cv::Mat &img_src = m_cvmat[PIC_ORIGINAL];
  img_src = input.getCurrentFrame()
                .clone(); // todo: find a way to do without cloning the frame
  ICOV_ASSERT_MSG(img_src.total() > 0, "Cannot encode empty image");

  double mse_frame = -1.0, psnr = -1.0;

  UInt maxval = 0;

  // CONSTANTS
  const Size icuSize_luma =
      Size(m_params.icuSize_luma.width, m_params.icuSize_luma.height);
  const Size tuSize_luma =
      Size(m_params.tuSize_luma.width, m_params.tuSize_luma.height);

  // end constants

  // resize
  cv::Size size_before = img_src.size();
  resize_covering_blocks(img_src,
                         cv::Size(icuSize_luma.width, icuSize_luma.height));
  cv::Size size_after = img_src.size();

  if (size_after != size_before)
    std::cout << "image size is changing from " << size_before << " to "
              << size_after << ". image aspect ratio is changing from "
              << double(size_before.width) / double(size_before.height)
              << " to " << double(size_after.width) / double(size_after.height)
              << std::endl;

  const int sourceWidth = img_src.cols;
  const int sourceHeight = img_src.rows;
  // end resize

  UnitArea extendedFrameArea(
      m_params.chFormat,
      Area(0, 0, sourceWidth + 2 * m_params.margin_x_360luma, sourceHeight));
  UnitArea mainFrameArea(m_params.chFormat, Area(m_params.margin_x_360luma, 0,
                                                 sourceWidth, sourceHeight));

  //  PelStorage picTemp;
  //  picTemp.create(extendedFrameArea);

  m_vvc_bufs.create(PIC_ORIGINAL, extendedFrameArea);
  m_vvc_bufs.create(PIC_RECONSTRUCTION, extendedFrameArea);
  //  vvc_pic.create(PIC_PREDICTION, extendedFrameArea);

  std::vector<BlockPrediction::predType> predVec = {
      BlockPrediction::LEFT,         BlockPrediction::RIGHT,
      BlockPrediction::TOP,          BlockPrediction::BOTTOM,
      BlockPrediction::TOP_LEFT,     // 0b00101
      BlockPrediction::BOTTOM_LEFT,  // 0b01001
      BlockPrediction::BOTTOM_RIGHT, // 0b01010
      BlockPrediction::TOP_RIGHT};

  for (auto pk : predVec) {
    m_vvc_bufs.create(PIC_PREDICTION + pk, extendedFrameArea);
  }

  PelUnitBuf mainFrameOrig = m_vvc_bufs.get(PIC_ORIGINAL).getBuf(mainFrameArea);

  std::cout << mainFrameOrig.get(COMPONENT_Y).width << "=" << img_src.cols
            << std::endl;
  std::cout << mainFrameOrig.get(COMPONENT_Y).height << "=" << img_src.rows
            << std::endl;
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

  psnr = VTM_Utility::calculatePSNR(
      m_vvc_bufs.getBuf(PIC_ORIGINAL), m_vvc_bufs.getBuf(PIC_RECONSTRUCTION),
      m_params.internalBitDepth, &mse_frame, &maxval);

  std::cout << "PSNR=" << psnr << std::endl;

  // INTRA PRED
  PelUnitBuf mainFrameReco =
      m_vvc_bufs.get(PIC_RECONSTRUCTION).getBuf(mainFrameArea);

  icov::video::IntraPrediction intraPred;
  intraPred.init(m_params.chFormat);

  ICOV_LOG(info) << "Start computing intra predictions";
  for (UInt ch = 0; ch < m_numComp; ch++) {
    ICOV_LOG(info) << "Channel " << ch + 1 << " / " << m_numComp << std::endl;

    const ComponentID compID = ComponentID(ch);
    copyBordersFor360(mainFrameReco, compID);

    m_enc_struc.applyPredictionBasedOnEntropy(
        m_vvc_bufs, intraPred, compID, m_params.internalBitDepth,
        m_params.tuSizeAfterRegrouping_luma.width,
        m_params.tuSizeAfterRegrouping_luma.height, false, m_params.qp,
        m_params.maxLog2TrDynamicRange);

    //    storageInBit += MyUtility::computeRatesAndCalculateStorageInBit(
    //        m_enc_struc, compID, tuSizeAfterRegrouping_luma,
    //    &tempIntraModeBits,
    //        maxLog2TrDynamicRange + 1);
    //    totalRequiredBitsToCodeIntraModes += tempIntraModeBits;
  }
  ICOV_LOG(info) << "Successfully computed intra predictions" << std::endl;

  for (auto pk : predVec) {
    cv::Mat &intraMat = m_cvmat[PIC_PREDICTION + pk];
    VTM_OPENCV_Utility::toBGRMat(
        m_vvc_bufs.get(PIC_PREDICTION + pk).getBuf(mainFrameArea), intraMat);
    //    cv::namedWindow("intra_" + std::to_string(pk), cv::WINDOW_NORMAL);
    //    cv::imshow("intra_" + std::to_string(pk), intraMat);
  }
  // END INTRA PRED

  // VIEWER
  auto &m_cvMat_reconstructedEquiImage = m_cvmat[RecoViewID];
  auto &m_cvMat_blockSI = m_cvmat[PredViewID];

  // draw lines on reco image
  VTM_OPENCV_Utility::toBGRMat(
      m_vvc_bufs.get(PIC_RECONSTRUCTION).getBuf(mainFrameArea),
      m_cvmat[PIC_RECONSTRUCTION]);

  m_cvMat_reconstructedEquiImage = m_cvmat[PIC_RECONSTRUCTION].clone();

  for (uint bi = 0; bi <= m_enc_struc.n_icusPerCol; bi++) {
    cv::line(m_cvMat_reconstructedEquiImage,
             cv::Point(0, bi * icuSize_luma.height),
             cv::Point(m_cvMat_reconstructedEquiImage.cols,
                       bi * icuSize_luma.height),
             cv::Scalar(0, 255, 0), 1);
  }
  for (uint bj = 0; bj <= m_enc_struc.n_icusPerRow; bj++) {
    cv::line(
        m_cvMat_reconstructedEquiImage, cv::Point(bj * icuSize_luma.width, 0),
        cv::Point(bj * icuSize_luma.width, m_cvMat_reconstructedEquiImage.rows),
        cv::Scalar(0, 255, 0), 1);
  }
  // end draw lines

  cv::namedWindow(WIN_ORIG, cv::WINDOW_NORMAL);
  cv::namedWindow(WIN_RECO, cv::WINDOW_NORMAL);
  cv::namedWindow(WIN_PRED, cv::WINDOW_NORMAL);

  cv::setMouseCallback(WIN_RECO, callBackFuncViewPred, this);
  if (m_params.enable_block_encoding)
    cv::setMouseCallback(WIN_PRED, callBackFuncEncoding, this);

  cv::imshow(WIN_ORIG, input.getCurrentFrame());
  cv::imshow(WIN_RECO, m_cvMat_reconstructedEquiImage);

  m_cvMat_blockSI.create(3 * icuSize_luma.height, 3 * icuSize_luma.width,
                         m_cvMat_reconstructedEquiImage.type());

  m_cvMat_blockSI.setTo(cv::Scalar(255, 0, 255));
  m_cvMat_blockSI(cv::Rect(icuSize_luma.width, icuSize_luma.height,
                           icuSize_luma.width, icuSize_luma.height))
      .setTo(cv::Scalar(255, 0, 0));

  onSelectBlock(m_cvMat_reconstructedEquiImage.cols / 2,
                m_cvMat_reconstructedEquiImage.rows / 2);
  m_firstDisplayBlocks = false;

  // END VIEWER

  std::vector<cv::Mat> reco_preds_output;
  reco_preds_output.push_back(m_cvmat[PIC_RECONSTRUCTION]);
  for (auto pk : predVec) {
    reco_preds_output.push_back(m_cvmat[PIC_PREDICTION + pk]);
  }

  if (m_params.write_intra_pred_image)
    cv::imwrite("reco_preds_output.tiff", reco_preds_output);

  //  m_params.enable_block_encoding = false;

  m_cvmat[DecodedDebugViewID].create(icuSize_luma.height, icuSize_luma.width,
                                     m_cvMat_reconstructedEquiImage.type());

  cv::waitKey(0);
}

/*****************************************************************************/
void icov::encoder::DemoEncoder::encode_tu(
    std::ostream &output, const Block &b,
    const icov::video::IcuEncoder &icu_enc,
    const icov::video::SideInformationUnit &si, BlockPrediction::predType pred,
    ComponentID comp, int tu_index) {
  const auto &x_tu = icu_enc.get_sourceX().TUs[tu_index];
  auto x_tuCoeff = x_tu.getQuantizedTransformCoeffs(comp);

  size_t tu_size = x_tuCoeff.area();

  const auto &si_tu = si.TUs[tu_index];
  auto si_tuCoeff = si_tu.getQuantizedTransformCoeffs(comp);

  ICOV_ASSERT_MSG(
      (size_t)si_tuCoeff.area() == tu_size,
      "Transform units from Source and its SI must be the same size: " +
          std::to_string(tu_size));

  ICOV_ASSERT_MSG(m_coders.count(tu_size) > 0,
                  "Coder not found for the given size: " +
                      std::to_string(tu_size));

  BitSymbolProvider bsp(SymboleVec(x_tuCoeff.buf, x_tuCoeff.buf + tu_size));
  bsp.set_si(SIVec(si_tuCoeff.buf, si_tuCoeff.buf + tu_size),
             m_params.bsp_params);

  icov::SINumcodeIdx si_numcodes;
  icov::SISyndromes si_synd;

  m_coders.at(tu_size)->encode_bitplanes(bsp, si_numcodes, si_synd);

  BitSymbolProvider bsp_dec(tu_size);
  bsp_dec.set_si(bsp.si_symboles(), bsp.z_min(), bsp.z_max(), bsp.q0(),
                 bsp.x_binary_length());

  for (size_t i = 0; i < si_numcodes.size(); i++) {
    output << b.getId(Block::SELF) << "\t" << b.parent()->row(b.v()) << "\t"
           << b.parent()->col(b.u()) << "\t" << BlockPrediction::toString(pred)
           << "\t" << tu_index << "\t" << (int)comp << "\t" << i << "\t"
           << m_coders.at(tu_size)->get_r(si_numcodes[i]) << "\t"
           << si_synd[i].to_string(" ") << "\t";
    output << std::endl;

    if (m_params.check_decoding) {

      ICOV_LOG(debug) << "Check decoding bitplane " << i << std::endl;

      BitVec decoded;

      try {
        ICOV_LOG(debug) << "Check bitplane " << i;
        m_coders.at(tu_size)->decode_bitplane(decoded, i, bsp_dec,
                                              si_numcodes[i], si_synd[i]);
        icov::helpers::assert::check(decoded == bsp.src_bits()[i],
                                     "Decoding BP check failed " +
                                         std::to_string(i));
      } catch (const std::exception &e) {
        ICOV_LOG(error) << e.what();
        break;
      }
    }
  }

  if (m_params.check_decoding) {
    try {
      ICOV_LOG(debug) << "Decode channel " << comp;

      bsp_dec.set_si(bsp.si_symboles(), bsp.z_min(), bsp.z_max(), bsp.q0(),
                     bsp.x_binary_length());
      m_coders.at(tu_size)->decode_symboles(bsp_dec, si_numcodes, si_synd);

      icov::helpers::assert::check(bsp_dec.src_symboles() == bsp.src_symboles(),
                                   "Decoding symbole failed for channel " +
                                       std::to_string(comp));

      ICOV_LOG(info) << "Channel was successfully decoded";
    } catch (const std::exception &e) {
      ICOV_LOG(error) << e.what();
    }
  }
}

void icov::encoder::DemoEncoder::encode_si(
    std::ostream &output, const Block &b,
    const icov::video::IcuEncoder &icu_enc,
    const icov::video::SideInformationUnit &si, BlockPrediction::predType pred,
    ComponentID comp) {
  for (size_t tu_index = 0; tu_index < si.TUs.size(); tu_index++) {
    encode_tu(output, b, icu_enc, si, pred, comp, tu_index);
  }
  ICOV_LOG(info) << "Block was successfully encoded";

  if (m_params.check_decoding) {
    ICOV_LOG(info) << "Decoding check passed";
  }
}

void icov::encoder::DemoEncoder::encode_icu(
    std::ostream &output, const Block &b,
    const icov::video::IcuEncoder &icu_enc, BlockPrediction::predType pred) {

  ICOV_LOG(info) << "Encode ICU with prediction "
                 << BlockPrediction::toString(pred) << std::endl;

  auto si = icu_enc.get_sideInformation(pred);
  icov::helpers::assert::not_null(si);
  for (uint c = 0; c < m_numComp; c++) {
    encode_si(output, b, icu_enc, *si, pred, static_cast<ComponentID>(c));
  }
}

/*****************************************************************************/
void DemoEncoder::encodeBlockWithPrediction(std::ostream &output,
                                            const Block &b,
                                            uint16_t prediction) {

  BlockPrediction::predType pred = BlockPrediction::toPredType(prediction);

  ICOV_LOG(debug) << "try encode prediction " << BlockPrediction::toString(pred)
                  << std::endl;

  int x_block = b.parent()->col(b.u());
  int y_block = b.parent()->row(b.v());

  Position pos(static_cast<float>(x_block) /
                   static_cast<float>(m_params.icuSize_luma.width),
               static_cast<float>(y_block) /
                   static_cast<float>(m_params.icuSize_luma.height));

  ICOV_LOG(trace) << pos.x;
  ICOV_LOG(trace) << pos.y;

  const auto &es = this->m_enc_struc;
  uint icuIndex = es.get_icuIndex(pos);

  ICOV_LOG(trace) << icuIndex;

  const auto &icu_enc = es.getICU(icuIndex);
  const auto &si_map = icu_enc.get_intraSIsMap();

  if (si_map.count(pred) > 0)
    encode_icu(output, b, icu_enc, pred);
}

void icov::encoder::DemoEncoder::encode(
    std::ostream &output, IInputMedia<cv::Mat> &input, const IPicture &picture,
    const IAccessBlockManager &access_blocks,
    const IBlockPrediction &block_prediction) {
  m_pic_ptr = &picture;
  IFrameEncoder::encode(output, input, picture, access_blocks,
                        block_prediction);
  m_pic_ptr = nullptr;
}
