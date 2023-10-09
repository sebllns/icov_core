/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/
#include "vtm_opencv_utility.h"

#include "icov_assert.h"
#include "icov_exception.h"
#include "rgb888_ycrcb.h"
#include <limits>
#include <opencv2/imgproc.hpp>

namespace VTM_OPENCV_Utility {

double mse(const cv::Mat &I1, const cv::Mat &I2)
{
    icov::assert::check((I1.type() == I2.type()) && (I1.size == I2.size),
                        ICOV_DEBUG_HEADER("Matrices must have the same types and sizes"));

    cv::Mat s1;
    absdiff(I1, I2, s1);                         // |I1 - I2|
    s1.convertTo(s1, CV_32F);                    // cannot make a square on 8 bits
    s1 = s1.mul(s1);                             // |I1 - I2|^2
    cv::Scalar s = sum(s1);                      // sum elements per channel
    double sse = s.val[0] + s.val[1] + s.val[2]; // sum channels
    if (sse <= 1e-10)                            // for small values return zero
        return 0;
    else
        return sse / (double)(I1.channels() * I1.total());
}

double psnr(const cv::Mat &I1, const cv::Mat &I2) {
  double mse_val = mse(I1, I2);
  if (mse_val == 0)
    return std::numeric_limits<double>::max();
  return 10.0 * log10((255 * 255) / mse_val);
}

void toBGRMat(const PelUnitBuf &src, cv::Mat &output) {
  const ChromaFormat format = src.chromaFormat;
  //    const UInt          numValidComp = ::getNumberValidComponents(format);

  cv::Mat Y = toMat_16S(src, COMPONENT_Y);

  switch (format) {
  case CHROMA_400: {
    Y.convertTo(output, CV_8UC1);
    //        output.create(Y.size(), CV_8UC1);

    //        for(int y=0; y < output.rows; y++)
    //        {
    //            uchar* data_row_cvMat = output.ptr<uchar>(y);
    //            const Pel* data_row_buf = src.get(COMPONENT_Y).buf +
    //            src.get(COMPONENT_Y).stride * y; for(int x=0; x < output.cols;
    //            x++)
    //                data_row_cvMat[x] = uchar();
    //                data_row_buf[x] = Pel(data_row_cvMat[x]);

    //        }
  } break;
  case CHROMA_420: {
    output.create(Y.size(), CV_8UC3);
    const CPelBuf areaBuf_Y = src.get(COMPONENT_Y);
    const CPelBuf areaBuf_Cr = src.get(COMPONENT_Cr);
    const CPelBuf areaBuf_Cb = src.get(COMPONENT_Cb);

    cv::parallel_for_(
        cv::Range(0, Y.rows / 2),
        YCrCb420p2BGR888Invoker<2>(output, areaBuf_Y.buf, areaBuf_Cr.buf,
                                   areaBuf_Cb.buf, areaBuf_Y.stride,
                                   areaBuf_Cr.stride, areaBuf_Cb.stride));
  } break;
  case CHROMA_422: // TODO: CHROMA_422 not tested
  {
    Y.copyTo(output);
    cv::Mat Cb = toMat_16S(src, COMPONENT_Cb);
    cv::Mat Cb_resized;
    Cb.assignTo(Cb_resized, CV_8UC1);
    if (Y.size() != Cb.size())
      cv::resize(Cb_resized, Cb_resized, Y.size(), 0, 0, cv::INTER_CUBIC);

    cv::Mat Cr = toMat_16S(src, COMPONENT_Cr);
    cv::Mat Cr_resized;

    Cr.assignTo(Cr_resized, CV_8UC1);
    if (Y.size() != Cr.size())
      cv::resize(Cr_resized, Cr_resized, Y.size(), 0, 0, cv::INTER_CUBIC);

    cv::Mat Y_8U;
    Y.assignTo(Y_8U, CV_8UC1);
    std::vector<cv::Mat> yuvVec;
    yuvVec.push_back(Y_8U);       // O(1) operation
    yuvVec.push_back(Cr_resized); // O(1) operation
    yuvVec.push_back(Cb_resized); // O(1) operation
    cv::Mat yuv_mat;
    cv::merge(yuvVec, yuv_mat);
    cvtColor(yuv_mat, output, cv::COLOR_YCrCb2BGR);
    //        yuv_mat.copyTo(output);

  } break;
  case CHROMA_444: {
    //        cv::Mat Cb;
    //        toMat_16S(src, COMPONENT_Cb).assignTo(Cb, CV_8UC1);

    //        cv::Mat Cr;
    //        toMat_16S(src, COMPONENT_Cr).assignTo(Cr, CV_8UC1);

    //        cv::Mat Y_8U;
    //        Y.assignTo(Y_8U, CV_8UC1);
    //        std::vector<cv::Mat> yuvVec;
    //        yuvVec.push_back(Y_8U);    // O(1) operation
    //        yuvVec.push_back(Cr);    // O(1) operation
    //        yuvVec.push_back(Cb);    // O(1) operation
    //        cv::Mat yuv_mat;
    //        cv::merge(yuvVec, yuv_mat);
    //        cvtColor(yuv_mat, output, CV_YCrCb2BGR);

    output.create(Y.size(), CV_8UC3);
    const CPelBuf areaBuf_Y = src.get(COMPONENT_Y);
    const CPelBuf areaBuf_Cr = src.get(COMPONENT_Cr);
    const CPelBuf areaBuf_Cb = src.get(COMPONENT_Cb);

    cv::parallel_for_(
        cv::Range(0, Y.rows),
        YCrCb444p2BGR888Invoker<2>(output, areaBuf_Y.buf, areaBuf_Cr.buf,
                                   areaBuf_Cb.buf, areaBuf_Y.stride,
                                   areaBuf_Cr.stride, areaBuf_Cb.stride));
  } break;
  default:
    std::cerr << "chroma format not supported" << std::endl;
    break;
  }
}

// TODO CHECK HERE
void cvMatToYUV(const cv::Mat &input, PelUnitBuf &pic) {

  CHECK(pic.get(COMPONENT_Y).width != input.cols ||
            pic.get(COMPONENT_Y).height != input.rows,
        "sizes do not match");
  CHECK(input.depth() != CV_8U, "only depth 8u is accepted");

  if (pic.chromaFormat == CHROMA_420)
    cv::parallel_for_(cv::Range(0, input.rows / 2),
                      BGR888toYCrCb420pInvoker(
                          input, pic.get(COMPONENT_Y).buf,
                          pic.get(COMPONENT_Cb).buf, pic.get(COMPONENT_Cr).buf,
                          pic.get(COMPONENT_Y).stride,
                          pic.get(COMPONENT_Cb).stride,
                          pic.get(COMPONENT_Cr).stride, false, false));
  else if (pic.chromaFormat == CHROMA_400) {
    for (int y = 0; y < input.rows; y++) {
      const uchar *data_row_cvMat = input.ptr<uchar>(y);
      Pel *data_row_buf =
          pic.get(COMPONENT_Y).buf + pic.get(COMPONENT_Y).stride * y;
      for (int x = 0; x < input.cols; x++)
        data_row_buf[x] = Pel(data_row_cvMat[x]);
    }
  } else if (pic.chromaFormat == CHROMA_444) {
    CHECK(pic.get(COMPONENT_Cr).width != input.cols ||
              pic.get(COMPONENT_Cr).height != input.rows,
          "sizes do not match");
    CHECK(pic.get(COMPONENT_Cb).width != input.cols ||
              pic.get(COMPONENT_Cb).height != input.rows,
          "sizes do not match");
    cv::parallel_for_(cv::Range(0, input.rows),
                      BGR888toYCrCb444pInvoker(
                          input, pic.get(COMPONENT_Y).buf,
                          pic.get(COMPONENT_Cb).buf, pic.get(COMPONENT_Cr).buf,
                          pic.get(COMPONENT_Y).stride,
                          pic.get(COMPONENT_Cb).stride,
                          pic.get(COMPONENT_Cr).stride, false, false));
  } else // TODO: implement for other formats
  {
    ICOV_THROW_STD_EXCEPTION("not implemented yet");
  }
}

} // namespace VTM_OPENCV_Utility
