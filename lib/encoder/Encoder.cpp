#include "Encoder.h"

#include "enc_helpers.h"
#include "omnidirectional_layer/BlockPrediction.h"
#include "rgb888_ycrcb.h"
#include "video_layer/encodingstructure.h"
#include "video_layer/mypicture.h"
#include "video_layer/vtm_opencv_utility.h"
#include "video_layer/vtm_utility.h"
#include <boost/filesystem.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

namespace fs = boost::filesystem;

Encoder::Encoder() {}

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
  ;
  UInt maxval = 0;

  // constants
  const Size icuSize_luma = Size(32, 32);
  const Size tuSize_luma = Size(32, 32);
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

  const ChromaFormat chromaFormatIDC = CHROMA_420;

  const int values[] = {img_src.rows, img_src.cols, img_src.type(),
                        img_src.channels()};
  output.write(reinterpret_cast<const char *>(&values), sizeof(values));

  const UInt numCh = getNumberValidComponents(chromaFormatIDC);

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
      chromaFormatIDC,
      Area(0, 0, sourceWidth + 2 * margin_x_360luma, sourceHeight));
  UnitArea mainFrameArea(chromaFormatIDC,
                         Area(margin_x_360luma, 0, sourceWidth, sourceHeight));

  PelStorage picTemp;
  picTemp.create(extendedFrameArea);

  MyPicture vvc_pic;
  vvc_pic.create(PIC_ORIGINAL, extendedFrameArea);
  vvc_pic.create(PIC_RECONSTRUCTION, extendedFrameArea);
  //  vvc_pic.create(PIC_PREDICTION, extendedFrameArea);

  std::vector<MyIntraPrediction::REFERENCE_SAMPLE_SIDE> predVec = {
      MyIntraPrediction::NO_SIDE,    MyIntraPrediction::LEFT,
      MyIntraPrediction::RIGHT,      MyIntraPrediction::TOP,
      MyIntraPrediction::DOWN,
      MyIntraPrediction::LEFT_TOP,   // 0b00101
      MyIntraPrediction::LEFT_DOWN,  // 0b01001
      MyIntraPrediction::RIGHT_DOWN, // 0b01010
      MyIntraPrediction::RIGHT_TOP};

  for (auto pk : predVec) {
    vvc_pic.create(PIC_PREDICTION + pk, extendedFrameArea);
  }

  PelUnitBuf mainFrameOrig = vvc_pic.get(PIC_ORIGINAL).getBuf(mainFrameArea);
  EncodingStructure enc_struc;

  std::cout << mainFrameOrig.get(COMPONENT_Y).width << "=" << img_src.cols
            << std::endl;
  std::cout << mainFrameOrig.get(COMPONENT_Y).height << "=" << img_src.rows
            << std::endl;
  cvMatToYUV(img_src, mainFrameOrig);
  enc_struc.create360VideoInteractiveCodingUnits_withoutAccessBlockComputation(
      chromaFormatIDC, sourceWidth, sourceHeight, margin_x_360luma,
      icuSize_luma.width, icuSize_luma.height, tuSize_luma.width,
      tuSize_luma.height);

  VTM_Utility::quantizeFrame(
      qp, vvc_pic.get(PIC_ORIGINAL), vvc_pic.get(PIC_RECONSTRUCTION), enc_struc,
      internalBitDepth,
      maxLog2TrDynamicRange); // give the whole extended image ( .getOrigBuf()
                              // and
  // .getRecoBuf() ) to the function because vector
  // icues_enc is based on extended frames

  psnr = VTM_Utility::calculatePSNR(vvc_pic.getBuf(PIC_ORIGINAL),
                                    vvc_pic.getBuf(PIC_RECONSTRUCTION),
                                    internalBitDepth, &mse_frame, &maxval);

  std::cout << "PSNR=" << psnr << std::endl;

  PelUnitBuf mainFrameReco =
      vvc_pic.get(PIC_RECONSTRUCTION).getBuf(mainFrameArea);
  //  PelUnitBuf mainFrameReco =
  //      vvc_pic.getBuf(PIC_RECONSTRUCTION).getBuf(mainFrameArea);
  // Intra prediction setup
  MyIntraPrediction intraPred;
  intraPred.init(chromaFormatIDC);

  std::cout << "Compute intra predictions" << std::endl;
  for (UInt ch = 0; ch < numCh; ch++) {
    std::cout << "Channel " << ch << std::endl << std::endl;
    const ComponentID compID = ComponentID(ch);
    copyBordersFor360(mainFrameReco, compID);

    enc_struc.applyPredictionBasedOnEntropy(
        vvc_pic, intraPred, compID, internalBitDepth,
        tuSizeAfterRegrouping_luma.width, tuSizeAfterRegrouping_luma.height,
        false, qp, maxLog2TrDynamicRange);

    //    storageInBit += MyUtility::computeRatesAndCalculateStorageInBit(
    //        enc_struc, compID, tuSizeAfterRegrouping_luma,
    //    &tempIntraModeBits,
    //        maxLog2TrDynamicRange + 1);
    //    totalRequiredBitsToCodeIntraModes += tempIntraModeBits;
  }
  std::cout << "END of Compute intra predictions" << std::endl;

  cv::Mat cvMat_reconstructedEquiImage;
  VTM_OPENCV_Utility::toBGRMat(mainFrameReco, cvMat_reconstructedEquiImage);

  cv::namedWindow("viewport original", cv::WINDOW_NORMAL);
  cv::imshow("viewport original", img_src);

  cv::namedWindow("viewport RECONSTRUCTED", cv::WINDOW_NORMAL);
  cv::imshow("viewport RECONSTRUCTED", cvMat_reconstructedEquiImage);

  cv::waitKey(0);

  for (auto pk : predVec) {

    cv::Mat intraMat;
    VTM_OPENCV_Utility::toBGRMat(vvc_pic.getBuf(PIC_PREDICTION + pk), intraMat);
    cv::namedWindow("intra_" + std::to_string(pk), cv::WINDOW_NORMAL);
    cv::imshow("intra_" + std::to_string(pk), intraMat);
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
