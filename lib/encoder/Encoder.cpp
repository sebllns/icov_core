#include "Encoder.h"

#include "enc_helpers.h"
#include "omnidirectional_layer/BlockPrediction.h"
#include "rgb888_ycrcb.h"
#include "video_layer/mypicture.h"
#include "video_layer/vtm_opencv_utility.h"
#include "video_layer/vtm_utility.h"
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

namespace fs = boost::filesystem;
using namespace icov;
using SI_SIDE = BlockPrediction::predType;
const int Encoder::BlockZoomID = PIC_PREDICTION + MyIntraPrediction::MAX;
const int Encoder::RecoViewID = Encoder::BlockZoomID + 1;

void copyBordersFor360(PelUnitBuf &mainFrame_roi, const ComponentID compID) {
  PelBuf &compBuf = mainFrame_roi.get(compID);

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

void callBackFunc(int event, int x, int y, int flags, void *userdata) {
  static_cast<Encoder *>(userdata)->onMouse(event, x, y, flags);
}

Encoder::Encoder()
    : m_firstDisplayBlocks(true), m_icuSize_luma(32, 32), m_tuSize_luma(32, 32),
      m_chromaFormatIDC(CHROMA_420),
      m_numComp(getNumberValidComponents(m_chromaFormatIDC)) {}

Encoder::~Encoder() { std::cout << "destroy encoder" << std::endl; }

void Encoder::onMouse(int event, int x, int y, int flags) {
  if (event == cv::EVENT_LBUTTONDOWN) {
    int bx = x / m_icuSize_luma.width;
    int by = y / m_icuSize_luma.height;
    if (bx < (int)m_enc_struc.n_icusPerRow &&
        by < (int)m_enc_struc.n_icusPerCol)
      onSelectBlock(bx * m_icuSize_luma.width, by * m_icuSize_luma.height);
  }
}

void Encoder::onSelectBlock(int x, int y) {
  cv::Point bp = cv::Point(x, y);

  if (bp != m_currentBlockPosition) {

    auto &m_cvMat_reconstructedEquiImage = m_cvmat[RecoViewID];
    auto &m_cvMat_blockSI = m_cvmat[BlockZoomID];

    if (!m_firstDisplayBlocks) {
      m_cvMat_blockSI(
          cv::Rect(m_icuSize_luma.width + 1, m_icuSize_luma.height + 1,
                   m_icuSize_luma.width - 1, m_icuSize_luma.height - 1))
          .copyTo(m_cvMat_reconstructedEquiImage(
              cv::Rect(m_currentBlockPosition + cv::Point(1, 1),
                       m_icuSize_luma - cv::Size(1, 1))));
    }

    m_cvMat_blockSI.setTo(0);

    int x_block = x, y_block = y;

    auto &es = this->m_enc_struc;
    uint icuIndex = es.get_icuIndex(Position(x_block / m_icuSize_luma.width,
                                             y_block / m_icuSize_luma.height));
    auto &icu_enc = es.getICU(icuIndex);
    auto &si_map = icu_enc.get_intraSIsMap();

    cv::Rect rect_src(x_block, y_block, m_icuSize_luma.width,
                      m_icuSize_luma.height);
    cv::Rect rect_dst(rect_src.width, rect_src.height, rect_src.width,
                      rect_src.height);

    cv::Mat src_block = m_cvmat[PIC_RECONSTRUCTION](rect_src);

    src_block.copyTo(m_cvMat_blockSI(rect_dst));

    for (const auto &si : si_map) {

      cv::Mat si_block = m_cvmat[PIC_PREDICTION + si.first](rect_src);

      int bi = 0, bj = 0;

      std::stringstream predStr;

      if (si.first & MyIntraPrediction::TOP) {
        predStr << "TOP ";
        bi = -1;
      }
      if (si.first & MyIntraPrediction::DOWN) {
        predStr << "BOTTOM ";
        bi = 1;
      }
      if (si.first & MyIntraPrediction::LEFT) {
        predStr << "LEFT ";
        bj = -1;
      }
      if (si.first & MyIntraPrediction::RIGHT) {
        predStr << "RIGHT ";
        bj = 1;
      }

      int ybi = bi * m_icuSize_luma.height;
      int xbj = bj * m_icuSize_luma.width;

      double psnr = VTM_OPENCV_Utility::psnr(src_block, si_block);
      //      std::cout << "\t\t\t| " << psnr;

      std::cout << boost::format("%_20s  | %-8.2f ") % predStr.str() % psnr;

      for (uint i = 0; i < m_numComp; i++) {
        std::cout << boost::format("| %-8d") % (uint)(si.second.intraDir[i]);
        //        std::cout << "\t|\t" << (uint)(si.second.intraDir[i]);
      }

      std::cout << std::endl;

      rect_dst = cv::Rect(xbj + rect_src.width, ybi + rect_src.height,
                          rect_src.width, rect_src.height);

      si_block.copyTo(m_cvMat_blockSI(rect_dst));
      //      cv::putText(
      //          m_cvMat_blockSI, std::to_string(round(mse * 100) /
      //          100).substr(0, 4), rect_dst.tl(), cv::FONT_HERSHEY_PLAIN, 1,
      //          cv::Scalar(0, 0, 255), 3);
    }

    std::cout
        << "-------------------------------------------------------------------"
        << std::endl;

    m_cvMat_reconstructedEquiImage(cv::Rect(bp, m_icuSize_luma))
        .setTo(cv::Scalar(0, 255, 0));

    cv::imshow("viewport RECONSTRUCTED", m_cvMat_reconstructedEquiImage);
    cv::imshow("block view", m_cvMat_blockSI);

    m_currentBlockPosition = bp;
  }
}

void Encoder::openFile(const std::string &filename) {
  std::cout << "open file to encode " << filename << std::endl;
  fs::path filepath = fs::complete(fs::path(filename));
  ICOV_ASSERT_MSG(fs::is_regular_file(filepath),
                  "File not found: " + filepath.string());
  cv::Mat &img_src = m_cvmat[PIC_ORIGINAL];
  img_src = cv::imread(filename, -1);
  ICOV_WARNING_MSG(img_src.total() > 0, "Empty image");
}

void Encoder::dispose() {}

void Encoder::encode(std::ostream &output, const IPicture &picture,
                     const IAccessBlockManager &access_blocks) {

  cv::Mat &img_src = m_cvmat[PIC_ORIGINAL];

  ICOV_ASSERT_MSG(img_src.total() > 0, "Cannot encode empty image");

  double mse_frame = -1.0, psnr = -1.0;

  UInt maxval = 0;

  // CONSTANTS
  const Size icuSize_luma = Size(m_icuSize_luma.width, m_icuSize_luma.height);
  const Size tuSize_luma = Size(m_tuSize_luma.width, m_tuSize_luma.height);
  const Size tuSizeAfterRegrouping_luma = Size(1, 1);
  const int maxLog2TrDynamicRange = 15;
  const int margin_x_360luma = 2; // in fact we need 1 margin for each color
                                  // component, but since UnitArea
  // handles division by 2 for chroma 420, it is easier to add 2 pixel
  // margins per left and right side and this will be divided by 2 for
  // chroma.

  const std::vector<int> qp_set = {22, 27, 32, 37, 42};
  const int qp = 42;
  std::cout << "qp= " << qp << std::endl;

  const Int internalBitDepth[MAX_NUM_CHANNEL_TYPE] = {8, 8};
  // internalBitDepth[CHANNEL_TYPE_LUMA] = 8;   // Bit-depth the codec operates
  // at (input/output files will be converted). (default: MSBExtendedBitDepth).
  // If different to MSBExtendedBitDepth, source data will be converted
  // internalBitDepth[CHANNEL_TYPE_CHROMA] = 8;   // Bit-depth the codec
  // operates at (input/output files will be converted). (default:
  // MSBExtendedBitDepth). If different to MSBExtendedBitDepth, source data will
  // be converted

  const int values[] = {img_src.rows, img_src.cols, img_src.type(),
                        img_src.channels()};
  output.write(reinterpret_cast<const char *>(&values), sizeof(values));

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
      m_chromaFormatIDC,
      Area(0, 0, sourceWidth + 2 * margin_x_360luma, sourceHeight));
  UnitArea mainFrameArea(m_chromaFormatIDC,
                         Area(margin_x_360luma, 0, sourceWidth, sourceHeight));

  PelStorage picTemp;
  picTemp.create(extendedFrameArea);

  MyPicture vvc_pic;
  vvc_pic.create(PIC_ORIGINAL, extendedFrameArea);
  vvc_pic.create(PIC_RECONSTRUCTION, extendedFrameArea);
  //  vvc_pic.create(PIC_PREDICTION, extendedFrameArea);

  std::vector<MyIntraPrediction::REFERENCE_SAMPLE_SIDE> predVec = {
      MyIntraPrediction::LEFT,       MyIntraPrediction::RIGHT,
      MyIntraPrediction::TOP,        MyIntraPrediction::DOWN,
      MyIntraPrediction::LEFT_TOP,   // 0b00101
      MyIntraPrediction::LEFT_DOWN,  // 0b01001
      MyIntraPrediction::RIGHT_DOWN, // 0b01010
      MyIntraPrediction::RIGHT_TOP};

  for (auto pk : predVec) {
    vvc_pic.create(PIC_PREDICTION + pk, extendedFrameArea);
  }

  PelUnitBuf mainFrameOrig = vvc_pic.get(PIC_ORIGINAL).getBuf(mainFrameArea);

  std::cout << mainFrameOrig.get(COMPONENT_Y).width << "=" << img_src.cols
            << std::endl;
  std::cout << mainFrameOrig.get(COMPONENT_Y).height << "=" << img_src.rows
            << std::endl;
  cvMatToYUV(img_src, mainFrameOrig);
  m_enc_struc
      .create360VideoInteractiveCodingUnits_withoutAccessBlockComputation(
          m_chromaFormatIDC, sourceWidth, sourceHeight, margin_x_360luma,
          icuSize_luma.width, icuSize_luma.height, tuSize_luma.width,
          tuSize_luma.height);

  VTM_Utility::quantizeFrame(
      qp, vvc_pic.get(PIC_ORIGINAL), vvc_pic.get(PIC_RECONSTRUCTION),
      m_enc_struc, internalBitDepth,
      maxLog2TrDynamicRange); // give the whole extended image ( .getOrigBuf()
  // and
  // .getRecoBuf() ) to the function because vector
  // icues_enc is based on extended frames

  psnr = VTM_Utility::calculatePSNR(vvc_pic.getBuf(PIC_ORIGINAL),
                                    vvc_pic.getBuf(PIC_RECONSTRUCTION),
                                    internalBitDepth, &mse_frame, &maxval);

  std::cout << "PSNR=" << psnr << std::endl;

  // INTRA PRED
  PelUnitBuf mainFrameReco =
      vvc_pic.get(PIC_RECONSTRUCTION).getBuf(mainFrameArea);

  MyIntraPrediction intraPred;
  intraPred.init(m_chromaFormatIDC);

  std::cout << "Compute intra predictions" << std::endl;
  for (UInt ch = 0; ch < m_numComp; ch++) {
    std::cout << "Channel " << ch + 1 << " / " << m_numComp << std::endl
              << std::endl;
    const ComponentID compID = ComponentID(ch);
    copyBordersFor360(mainFrameReco, compID);

    m_enc_struc.applyPredictionBasedOnEntropy(
        vvc_pic, intraPred, compID, internalBitDepth,
        tuSizeAfterRegrouping_luma.width, tuSizeAfterRegrouping_luma.height,
        false, qp, maxLog2TrDynamicRange);

    //    storageInBit += MyUtility::computeRatesAndCalculateStorageInBit(
    //        m_enc_struc, compID, tuSizeAfterRegrouping_luma,
    //    &tempIntraModeBits,
    //        maxLog2TrDynamicRange + 1);
    //    totalRequiredBitsToCodeIntraModes += tempIntraModeBits;
  }
  std::cout << "END of Compute intra predictions" << std::endl;

  for (auto pk : predVec) {
    cv::Mat &intraMat = m_cvmat[PIC_PREDICTION + pk];
    VTM_OPENCV_Utility::toBGRMat(
        vvc_pic.get(PIC_PREDICTION + pk).getBuf(mainFrameArea), intraMat);
    //    cv::namedWindow("intra_" + std::to_string(pk), cv::WINDOW_NORMAL);
    //    cv::imshow("intra_" + std::to_string(pk), intraMat);
  }
  // END INTRA PRED

  // VIEWER
  auto &m_cvMat_reconstructedEquiImage = m_cvmat[RecoViewID];
  auto &m_cvMat_blockSI = m_cvmat[BlockZoomID];

  // draw lines on reco image
  VTM_OPENCV_Utility::toBGRMat(
      vvc_pic.get(PIC_RECONSTRUCTION).getBuf(mainFrameArea),
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

  //  cv::namedWindow("viewport original", cv::WINDOW_NORMAL);
  //  cv::imshow("viewport original", img_src);

  cv::namedWindow("viewport RECONSTRUCTED", cv::WINDOW_NORMAL);

  // ERROR HERE
  cv::imshow("viewport RECONSTRUCTED", m_cvMat_reconstructedEquiImage);
  cv::setMouseCallback("viewport RECONSTRUCTED", callBackFunc, this);

  m_cvMat_blockSI.create(3 * icuSize_luma.height, 3 * icuSize_luma.width,
                         m_cvMat_reconstructedEquiImage.type());

  m_cvMat_blockSI.setTo(cv::Scalar(255, 0, 255));
  m_cvMat_blockSI(cv::Rect(icuSize_luma.width, icuSize_luma.height,
                           icuSize_luma.width, icuSize_luma.height))
      .setTo(cv::Scalar(255, 0, 0));

  cv::namedWindow("block view", cv::WINDOW_NORMAL);

  onSelectBlock(m_cvMat_reconstructedEquiImage.cols / 2,
                m_cvMat_reconstructedEquiImage.rows / 2);
  m_firstDisplayBlocks = false;

  // END VIEWER

  std::vector<cv::Mat> reco_preds_output;
  reco_preds_output.push_back(m_cvmat[PIC_RECONSTRUCTION]);
  for (auto pk : predVec) {
    reco_preds_output.push_back(m_cvmat[PIC_PREDICTION + pk]);
  }
  cv::imwrite("reco_preds_output.tiff", reco_preds_output);

  cv::waitKey(0);

  //     //encode the blocks
  //     for(const auto& b: picture.icus())
  //     {
  //         encodeBlock(output, b,
  //         access_blocks.isAccessBlock(b.getId(Block::THIS)));
  //     }
}

void Encoder::encodeBlock(std::ostream &output, const Block &b,
                          bool access_block) {
  std::cout << "-- block " << b.center_u() << ", " << b.center_v() << std::endl;

  // if(access_block) encodeBlockWithPrediction(output, b,
  // BlockPrediction::NONE);

  std::vector<BlockPrediction::predType> pred_values = {
      BlockPrediction::RIGHT,        BlockPrediction::LEFT,
      BlockPrediction::TOP,          BlockPrediction::BOTTOM,
      BlockPrediction::TOP_RIGHT,    BlockPrediction::TOP_LEFT,
      BlockPrediction::BOTTOM_RIGHT, BlockPrediction::BOTTOM_LEFT,
  };

  for (auto pred : pred_values) {
    encodeBlockWithPrediction(output, b, static_cast<uint16_t>(pred));
  }
}

void Encoder::encodeBlockWithPrediction(std::ostream &output, const Block &b,
                                        uint16_t prediction) {
  std::cout << "---- prediction " << prediction << std::endl;

  const IPicture &p = *(b.parent());
  int bw = (int)(b.w() * p.width());
  int bh = (int)(b.h() * p.height());
  int bx = (int)(p.u(p.angleX(b.u())) * p.width());
  int by = (int)(p.v(p.angleY(b.v())) * p.height());
  auto rect_block = cv::Rect(bx, by, bw, bh);
}
