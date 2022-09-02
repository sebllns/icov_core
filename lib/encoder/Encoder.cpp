#include "Encoder.h"

#include "enc_helpers.h"
#include "omnidirectional_layer/BlockPrediction.h"
#include "rgb888_ycrcb.h"
#include "video_layer/ICOV_Picture.h"
#include "video_layer/vtm_opencv_utility.h"
#include "video_layer/vtm_utility.h"

#include <boost/filesystem.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

namespace fs = boost::filesystem;
using namespace icov;
using SI_SIDE = BlockPrediction::predType;

Encoder::Encoder() : m_icuSize_luma(32, 32), m_tuSize_luma(32, 32) {}

Encoder::~Encoder() { std::cout << "destroy encoder" << std::endl; }

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

    m_cvMat_blockSI(cv::Rect(m_icuSize_luma.width, m_icuSize_luma.height,
                             m_icuSize_luma.width, m_icuSize_luma.height))
        .copyTo(m_cvMat_reconstructedEquiImage(
            cv::Rect(m_currentBlockPosition, m_icuSize_luma)));

    int x_block = x, y_block = y;

    auto &es = this->m_enc_struc;
    uint icuIndex = es.get_icuIndex(Position(x_block / m_icuSize_luma.width,
                                             y_block / m_icuSize_luma.height));
    auto &icu_enc = es.getICU(icuIndex);
    auto &si_map = icu_enc.get_intraSIsMap();

    for (int bi = -1; bi < 2; bi++) {
      int ybi = bi * m_icuSize_luma.height;

      for (int bj = -1; bj < 2; bj++) {
        int xbj = bj * m_icuSize_luma.width;

        cv::Mat src;
        SI_SIDE side = SI_SIDE::UNKNOWN;
        if (bi < 0 && bj < 0) {
          side = SI_SIDE::TOP_LEFT;
        } else if (bi < 0 && bj == 0) {
          side = SI_SIDE::TOP;
        } else if (bi < 0 && bj > 0) {
          side = SI_SIDE::TOP_RIGHT;
        } else if (bi == 0 && bj < 0) {
          side = SI_SIDE::LEFT;
        } else if (bi == 0 && bj > 0) {
          side = SI_SIDE::RIGHT;
        } else if (bi > 0 && bj < 0) {
          side = SI_SIDE::BOTTOM_LEFT;
        } else if (bi > 0 && bj == 0) {
          side = SI_SIDE::BOTTOM;
        } else if (bi > 0 && bj > 0) {
          side = SI_SIDE::BOTTOM_RIGHT;
        } else if (bi == 0 && bj == 0) {
          side = SI_SIDE::NONE;
        } else {
          side = SI_SIDE::UNKNOWN;
        }

        ICOV_ASSERT_MSG(side != SI_SIDE::UNKNOWN, "SI side is undefined");

        if (side == SI_SIDE::NONE) {
          src = m_cvMat_reconstructedEquiImage(
              cv::Rect(xbj + x_block, ybi + y_block, m_icuSize_luma.width,
                       m_icuSize_luma.height));
        } else {

          double mse_frame = -1.0, psnr = -1.0;
          UInt maxval = 0;
          const Int internalBitDepth[MAX_NUM_CHANNEL_TYPE] = {8, 8};

          //          psnr = VTM_Utility::calculatePSNR(
          //              icu_enc.get_sourceX().TUs[0].getQuantizedTcoeffs(),
          //              si_map.at(side).TUs[0].getQuantizedTcoeffs(),
          //              internalBitDepth, &mse_frame, &maxval);

          psnr = VTM_Utility::calculatePSNR(
              icu_enc.get_sourceX().TUs[0].getQuantizedTcoeffs(),
              si_map.at(side).TUs[0].getQuantizedTcoeffs(), internalBitDepth,
              &mse_frame, &maxval);

          std::cout << "side(i,j) " << bi << ", " << bj << " psnr=" << psnr
                    << " mse_frame=" << mse_frame << " maxval=" << maxval
                    << std::endl;

          VTM_OPENCV_Utility::toBGRMat(
              si_map.at(side).TUs[0].getQuantizedTcoeffs(), src);
        }

        src.copyTo(m_cvMat_blockSI(
            cv::Rect(xbj + m_icuSize_luma.width, ybi + m_icuSize_luma.height,
                     m_icuSize_luma.width, m_icuSize_luma.height)));
      }
    }

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
  m_img_src = cv::imread(filename, -1);
  ICOV_WARNING_MSG(m_img_src.total() > 0, "Empty image");
}

void Encoder::dispose() {}

///**************************************************************************************
///
/// \brief Encoder::encode
/// \param output
/// \param picture
/// \param access_blocks
///
void Encoder::encode(std::ostream &output, const IPicture &picture,
                     const IAccessBlockManager &access_blocks) {
  ICOV_ASSERT_MSG(m_img_src.total() > 0, "Cannot encode empty image");

  double mse_frame = -1.0, psnr = -1.0;

  UInt maxval = 0;

  // CONSTANTS
  const Size icuSize_luma = Size(m_icuSize_luma.width, m_icuSize_luma.height);
  const Size tuSize_luma = Size(m_tuSize_luma.width, m_tuSize_luma.height);
  //    const Size icuSize_luma=Size(256, 256);
  //    const Size tuSize_luma=Size(256, 256);
  const Size tuSizeAfterRegrouping_luma = Size(1, 1);
  const int maxLog2TrDynamicRange = 15;
  const int margin_x_360luma = 2; // in fact we need 1 margin for each color
                                  // component, but since UnitArea
  // handles division by 2 for chroma 420, it is easier to add 2 pixel
  // margins per left and right side and this will be divided by 2 for
  // chroma.

  const std::vector<int> qp_set = {22, 27, 32, 37, 42};
  const int qp = 32;
  std::cout << "qp= " << qp << std::endl;

  const Int internalBitDepth[MAX_NUM_CHANNEL_TYPE] = {8, 8};
  // internalBitDepth[CHANNEL_TYPE_LUMA] = 8;   // Bit-depth the codec
  // operates at (input/output files will be converted). (default:
  // MSBExtendedBitDepth). If different to MSBExtendedBitDepth, source data
  // will be converted internalBitDepth[CHANNEL_TYPE_CHROMA] = 8;   //
  // Bit-depth the codec operates at (input/output files will be converted).
  // (default: MSBExtendedBitDepth). If different to MSBExtendedBitDepth,
  // source data will be converted

  const ChromaFormat chromaFormatIDC = CHROMA_420;

  const int values[] = {m_img_src.rows, m_img_src.cols, m_img_src.type(),
                        m_img_src.channels()};
  output.write(reinterpret_cast<const char *>(&values), sizeof(values));

  const UInt numCh = getNumberValidComponents(chromaFormatIDC);

  // END CONSTANTS

  // resize
  cv::Size size_before = m_img_src.size();
  resize_covering_blocks(m_img_src,
                         cv::Size(icuSize_luma.width, icuSize_luma.height));
  cv::Size size_after = m_img_src.size();

  if (size_after != size_before)
    std::cout << "image size is changing from " << size_before << " to "
              << size_after << ". image aspect ratio is changing from "
              << double(size_before.width) / double(size_before.height)
              << " to " << double(size_after.width) / double(size_after.height)
              << std::endl;

  const int sourceWidth = m_img_src.cols;
  const int sourceHeight = m_img_src.rows;
  // end resize

  // trick: extend for 360
  UnitArea extendedFrameArea(
      chromaFormatIDC,
      Area(0, 0, sourceWidth + 2 * margin_x_360luma, sourceHeight));
  UnitArea mainFrameArea(chromaFormatIDC,
                         Area(margin_x_360luma, 0, sourceWidth, sourceHeight));

  // Create picture
  icov::video::Picture encodingPicture;
  encodingPicture.create(PIC_ORIGINAL, extendedFrameArea);
  encodingPicture.create(PIC_RECONSTRUCTION, extendedFrameArea);

  for (uint pk = BlockPrediction::NONE; pk < BlockPrediction::INTER; pk++) {
    encodingPicture.create(PIC_PREDICTION + pk, extendedFrameArea);
  }

  PelUnitBuf mainFrameOrig =
      encodingPicture.get(PIC_ORIGINAL).getBuf(mainFrameArea);

  //  std::cout << mainFrameOrig.get(COMPONENT_Y).width << "=" <<
  //  m_img_src.cols
  //            << std::endl;
  //  std::cout << mainFrameOrig.get(COMPONENT_Y).height << "=" <<
  //  m_img_src.rows
  //            << std::endl;

  cvMatToYUV(m_img_src, mainFrameOrig);
  m_enc_struc
      .create360VideoInteractiveCodingUnits_withoutAccessBlockComputation(
          chromaFormatIDC, sourceWidth, sourceHeight, margin_x_360luma,
          icuSize_luma.width, icuSize_luma.height, tuSize_luma.width,
          tuSize_luma.height);

  VTM_Utility::quantizeFrame(
      qp, encodingPicture.get(PIC_ORIGINAL),
      encodingPicture.get(PIC_RECONSTRUCTION), m_enc_struc, internalBitDepth,
      maxLog2TrDynamicRange); // give the whole extended image ( .getOrigBuf()
  // and
  // .getRecoBuf() ) to the function because vector
  // icues_enc is based on extended frames

  psnr = VTM_Utility::calculatePSNR(encodingPicture.get(PIC_ORIGINAL),
                                    encodingPicture.get(PIC_RECONSTRUCTION),
                                    internalBitDepth, &mse_frame, &maxval);

  std::cout << "PSNR=" << psnr << std::endl;

  // draw lines on reco image
  VTM_OPENCV_Utility::toBGRMat(
      encodingPicture.get(PIC_RECONSTRUCTION).getBuf(mainFrameArea),
      m_cvMat_reconstructedEquiImage);
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

  cv::namedWindow("viewport original", cv::WINDOW_NORMAL);
  cv::imshow("viewport original", m_img_src);

  cv::namedWindow("viewport RECONSTRUCTED", cv::WINDOW_NORMAL);
  cv::imshow("viewport RECONSTRUCTED", m_cvMat_reconstructedEquiImage);
  cv::setMouseCallback("viewport RECONSTRUCTED", callBackFunc, this);

  m_cvMat_blockSI.create(3 * icuSize_luma.height, 3 * icuSize_luma.width,
                         m_cvMat_reconstructedEquiImage.type());

  m_cvMat_blockSI.setTo(cv::Scalar(255, 0, 255));
  m_cvMat_blockSI(cv::Rect(icuSize_luma.width, icuSize_luma.height,
                           icuSize_luma.width, icuSize_luma.height))
      .setTo(cv::Scalar(255, 0, 0));

  PelUnitBuf mainFrameReco =
      encodingPicture.get(PIC_RECONSTRUCTION).getBuf(mainFrameArea);
  // Intra prediction setup
  icov::video::IntraPrediction intraPred;
  intraPred.init(chromaFormatIDC);

  std::map<BlockPrediction::predType, cv::Mat> siMat;

  for (UInt ch = 0; ch < numCh; ch++) {
    const ComponentID compID = ComponentID(ch);
    copyBordersFor360(mainFrameReco, compID);

    m_enc_struc.applyPredictionBasedOnEntropy(
        encodingPicture.get(PIC_RECONSTRUCTION),
        encodingPicture.get(PIC_PREDICTION), intraPred, compID,
        internalBitDepth, tuSizeAfterRegrouping_luma.width,
        tuSizeAfterRegrouping_luma.height, false, qp, maxLog2TrDynamicRange,
        &siMat);

    //    storageInBit += Utility::computeRatesAndCalculateStorageInBit(
    //        enc_struc, compID, tuSizeAfterRegrouping_luma,
    //        &tempIntraModeBits, maxLog2TrDynamicRange + 1);
    //    totalRequiredBitsToCodeIntraModes += tempIntraModeBits;
  }

  cv::namedWindow("block view", cv::WINDOW_NORMAL);
  onSelectBlock(m_cvMat_reconstructedEquiImage.cols / 2,
                m_cvMat_reconstructedEquiImage.rows / 2);

  //  cv::Mat cvMat_intrapred;
  //  VTM_OPENCV_Utility::toBGRMat(encodingPicture.getPredBuf(mainFrameArea),
  //                               cvMat_intrapred);

  for (const auto &si : siMat) {
    cv::namedWindow("cvMat_intrapred_" + std::to_string(si.first),
                    cv::WINDOW_NORMAL);
    cv::imshow("cvMat_intrapred_" + std::to_string(si.first), si.second);
  }

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
