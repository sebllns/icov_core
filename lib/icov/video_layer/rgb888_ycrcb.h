/****************************************************************************/
/** @addtogroup video_layer
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup video_layer
 *  @ingroup icov-core
 */

/** @file rgb888_ycrcb.h
 *  @brief rgb888_ycrcb
 *  @ingroup video_layer
 *  @author Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
 *  @author Sébastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.0.0
 *  @date 2018, 2022
 *  @copyright Copyright (C) 2021 by Inria. All rights reserved.
 * The copyright in this software is being made available under
 * the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 * <br/>
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * <br/>
 * <b> For using ICOV under terms that will not comply with the GNU
 * AGPL, please contact Inria about acquiring an ICOV Professional
 * Edition License. </b>
 * <br/>
 * See contact section in README file for more information.
 * <br/>
 * This software was developed at:
 * Inria Rennes - Bretagne Atlantique
 * Campus Universitaire de Beaulieu
 * 35042 Rennes Cedex
 * France
 * <br/>
 * If you have questions regarding the use of this software, please contact
 * Inria using the contact section in the README file.
 * <br/>
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *****************************************************************************/
#pragma once
#include "CommonLib/CommonDef.h"
#include <opencv2/core.hpp>

namespace icov::video {

struct BGR888toYCrCb420pInvoker : public cv::ParallelLoopBody {
  BGR888toYCrCb420pInvoker(const cv::Mat &inputImage, Pel *_y_data,
                           Pel *_cr_data, Pel *_cb_data, size_t _y_stride,
                           size_t _cr_stride, size_t _cb_stride, bool swapBlue_,
                           bool swapUV_)
      : _inputImage(inputImage), y_data(_y_data), cr_data(_cr_data),
        cb_data(_cb_data), y_stride(_y_stride), cr_stride(_cr_stride),
        cb_stride(_cb_stride), swapBlue(swapBlue_), swapCrCB(swapUV_) {}

  void operator()(const cv::Range &rowRange) const {

    const int cn = _inputImage.channels();
    for (int i = rowRange.start; i < rowRange.end; i++) {
      const uchar *brow0 = _inputImage.ptr<uchar>(2 * i);
      const uchar *grow0 = brow0 + 1;
      const uchar *rrow0 = brow0 + 2;
      const uchar *brow1 = _inputImage.ptr<uchar>(2 * i + 1);
      const uchar *grow1 = brow1 + 1;
      const uchar *rrow1 = brow1 + 2;
      if (swapBlue) {
        std::swap(brow0, rrow0);
        std::swap(brow1, rrow1);
      }

      Pel *y = y_data + y_stride * (2 * i);
      Pel *cb = cr_data + cr_stride * i;
      Pel *cr = cb_data + cb_stride * i;

      if (swapCrCB) {
        std::swap(cb, cr);
      }

      for (int j = 0, k = 0; j < _inputImage.cols * cn; j += 2 * cn, k++) {
        int r00 = rrow0[j];
        int g00 = grow0[j];
        int b00 = brow0[j];
        int r01 = rrow0[cn + j];
        int g01 = grow0[cn + j];
        int b01 = brow0[cn + j];
        int r10 = rrow1[j];
        int g10 = grow1[j];
        int b10 = brow1[j];
        int r11 = rrow1[cn + j];
        int g11 = grow1[cn + j];
        int b11 = brow1[cn + j];

        float y00 = 0.299 * r00 + 0.587 * g00 + 0.114 * b00;
        float y01 = 0.299 * r01 + 0.587 * g01 + 0.114 * b01;
        float y10 = 0.299 * r10 + 0.587 * g10 + 0.114 * b10;
        float y11 = 0.299 * r11 + 0.587 * g11 + 0.114 * b11;

        y[2 * k + 0] = std::min(std::max(0.f, y00), 255.f);
        y[2 * k + 1] = std::min(std::max(0.f, y01), 255.f);
        y[2 * k + y_stride + 0] = std::min(std::max(0.f, y10), 255.f);
        y[2 * k + y_stride + 1] = std::min(std::max(0.f, y11), 255.f);

        //                float u00 = -0.147 * r00 - 0.289 * g00 + 0.436 * b00;
        //                float v00 =  0.615 * r00 - 0.515 * g00 - 0.100 * b00;
        float cb00 = -0.169 * r00 - 0.331 * g00 + 0.499 * b00 + 128;
        float cr00 = 0.499 * r00 - 0.418 * g00 - 0.0813 * b00 + 128;

        cb[k] = std::min(std::max(0.f, cb00), 255.f);
        cr[k] = std::min(std::max(0.f, cr00), 255.f);
      }
    }
  }

  void convert() const {
    if (_inputImage.cols * _inputImage.rows >= 320 * 240)
      cv::parallel_for_(cv::Range(0, _inputImage.rows / 2), *this);
    else
      operator()(cv::Range(0, _inputImage.rows / 2));
  }

private:
  BGR888toYCrCb420pInvoker &operator=(const BGR888toYCrCb420pInvoker &);
  const cv::Mat &_inputImage;
  Pel *y_data, *cr_data, *cb_data;
  size_t y_stride, cr_stride, cb_stride;
  bool swapBlue;
  bool swapCrCB;
};

template <int bIdx, typename Pel>
struct YCrCb420p2BGR888Invoker : cv::ParallelLoopBody {
  cv::Mat &_output;
  const Pel *_y_data, *_cr_data, *_cb_data;
  size_t _y_stride, _cr_stride, _cb_stride;

  YCrCb420p2BGR888Invoker(cv::Mat &output, const Pel *y_data,
                          const Pel *cr_data, const Pel *cb_data,
                          size_t y_stride, size_t cr_stride, size_t cb_stride)
      : _output(output), _y_data(y_data), _cr_data(cr_data), _cb_data(cb_data),
        _y_stride(y_stride), _cr_stride(cr_stride), _cb_stride(cb_stride) {}

  void operator()(const cv::Range &range) const {
    const int rangeBegin = range.start * 2;
    const int rangeEnd = range.end * 2;

    const Pel *y1 = _y_data + rangeBegin * _y_stride;
    const Pel *cr1 = _cr_data + (range.start /*/ 2*/) * _cr_stride;
    const Pel *cb1 = _cb_data + (range.start /*/ 2*/) * _cb_stride;

    for (int j = rangeBegin; j < rangeEnd;
         j += 2, y1 += _y_stride * 2, cr1 += _cr_stride, cb1 += _cb_stride) {
      uchar *row1 = _output.ptr<uchar>(j);
      uchar *row2 = _output.ptr<uchar>(j + 1);
      const Pel *y2 = y1 + _y_stride;

      for (int i = 0; i < _output.cols / 2; i += 1, row1 += 6, row2 += 6) {
        Pel cr = cr1[i] - 128;
        Pel cb = cb1[i] - 128;

        const Pel &y00 = y1[2 * i];
        row1[2 - bIdx] = std::min(std::max(0.f, y00 + 1.772f * cb), 255.f);
        row1[1] =
            std::min(std::max(0.f, y00 - 0.344f * cb - 0.714f * cr), 255.f);
        row1[bIdx] = std::min(std::max(0.f, y00 + 1.402f * cr), 255.f);

        const Pel &y01 = y1[2 * i + 1];
        row1[5 - bIdx] = std::min(std::max(0.f, y01 + 1.772f * cb), 255.f);
        row1[4] =
            std::min(std::max(0.f, y01 - 0.344f * cb - 0.714f * cr), 255.f);
        row1[3 + bIdx] = std::min(std::max(0.f, y01 + 1.402f * cr), 255.f);

        const Pel &y10 = y2[2 * i];
        row2[2 - bIdx] = std::min(std::max(0.f, y10 + 1.772f * cb), 255.f);
        row2[1] =
            std::min(std::max(0.f, y10 - 0.344f * cb - 0.714f * cr), 255.f);
        row2[bIdx] = std::min(std::max(0.f, y10 + 1.402f * cr), 255.f);

        const Pel &y11 = y2[2 * i + 1];
        row2[5 - bIdx] = std::min(std::max(0.f, y11 + 1.772f * cb), 255.f);
        row2[4] =
            std::min(std::max(0.f, y11 - 0.344f * cb - 0.714f * cr), 255.f);
        row2[3 + bIdx] = std::min(std::max(0.f, y11 + 1.402f * cr), 255.f);

        //                std::cout << int(row1[2-bIdx]) << " " << int(row1[1])
        //                << " " << int(row1[bIdx]) << " " << int(row1[5-bIdx])
        //                << " " << int(row1[4]) << " " << int(row1[3+bIdx]) <<
        //                std::endl; std::cout << int(row2[2-bIdx]) << " " <<
        //                int(row2[1]) << " " << int(row2[bIdx]) << " " <<
        //                int(row2[5-bIdx]) << " " << int(row2[4]) << " " <<
        //                int(row2[3+bIdx]) << std::endl; std::cout <<
        //                "=====================" << std::endl;
      }
    }
  }
};

struct BGR888toYCrCb444pInvoker : public cv::ParallelLoopBody {
  BGR888toYCrCb444pInvoker(const cv::Mat &inputImage, Pel *_y_data,
                           Pel *_cr_data, Pel *_cb_data, size_t _y_stride,
                           size_t _cr_stride, size_t _cb_stride, bool swapBlue_,
                           bool swapUV_)
      : _inputImage(inputImage), y_data(_y_data), cr_data(_cr_data),
        cb_data(_cb_data), y_stride(_y_stride), cr_stride(_cr_stride),
        cb_stride(_cb_stride), swapBlue(swapBlue_), swapCrCB(swapUV_) {}

  void operator()(const cv::Range &rowRange) const {

    const int cn = _inputImage.channels();
    for (int i = rowRange.start; i < rowRange.end; i++) {
      const uchar *brow0 = _inputImage.ptr<uchar>(i);
      const uchar *grow0 = brow0 + 1;
      const uchar *rrow0 = brow0 + 2;
      if (swapBlue)
        std::swap(brow0, rrow0);

      Pel *y = y_data + y_stride * i;
      Pel *cb = cr_data + cr_stride * i;
      Pel *cr = cb_data + cb_stride * i;

      if (swapCrCB) {
        std::swap(cb, cr);
      }

      for (int j = 0, k = 0; j < _inputImage.cols * cn; j += cn, k++) {
        int r00 = rrow0[j];
        int g00 = grow0[j];
        int b00 = brow0[j];

        float y00 = 0.299 * r00 + 0.587 * g00 + 0.114 * b00;

        y[k] = std::min(std::max(0.f, y00), 255.f);

        //                float u00 = -0.147 * r00 - 0.289 * g00 + 0.436 * b00;
        //                float v00 =  0.615 * r00 - 0.515 * g00 - 0.100 * b00;
        float cb00 = -0.169 * r00 - 0.331 * g00 + 0.499 * b00 + 128;
        float cr00 = 0.499 * r00 - 0.418 * g00 - 0.0813 * b00 + 128;

        cb[k] = std::min(std::max(0.f, cb00), 255.f);
        cr[k] = std::min(std::max(0.f, cr00), 255.f);
      }
    }
  }

  void convert() const {
    if (_inputImage.cols * _inputImage.rows >= 320 * 240)
      cv::parallel_for_(cv::Range(0, _inputImage.rows), *this);
    else
      operator()(cv::Range(0, _inputImage.rows));
  }

private:
  BGR888toYCrCb444pInvoker &operator=(const BGR888toYCrCb444pInvoker &);
  const cv::Mat &_inputImage;
  Pel *y_data, *cr_data, *cb_data;
  size_t y_stride, cr_stride, cb_stride;
  bool swapBlue;
  bool swapCrCB;
};

template <int bIdx, typename Pel>
struct YCrCb444p2BGR888Invoker : cv::ParallelLoopBody {
  cv::Mat &_output;
  const Pel *_y_data, *_cr_data, *_cb_data;
  size_t _y_stride, _cr_stride, _cb_stride;

  YCrCb444p2BGR888Invoker(cv::Mat &output, const Pel *y_data,
                          const Pel *cr_data, const Pel *cb_data,
                          size_t y_stride, size_t cr_stride, size_t cb_stride)
      : _output(output), _y_data(y_data), _cr_data(cr_data), _cb_data(cb_data),
        _y_stride(y_stride), _cr_stride(cr_stride), _cb_stride(cb_stride) {}

  void operator()(const cv::Range &range) const {

    const Pel *y1 = _y_data + range.start * _y_stride;
    const Pel *cr1 = _cr_data + range.start * _cr_stride;
    const Pel *cb1 = _cb_data + range.start * _cb_stride;

    for (int j = range.start; j < range.end;
         j++, y1 += _y_stride, cr1 += _cr_stride, cb1 += _cb_stride) {
      uchar *row1 = _output.ptr<uchar>(j);

      for (int i = 0; i < _output.cols; i += 1, row1 += 3) {
        Pel cr = cr1[i] - 128;
        Pel cb = cb1[i] - 128;

        const Pel &y00 = y1[i];
        row1[2 - bIdx] = std::min(std::max(0.f, y00 + 1.772f * cb), 255.f);
        row1[1] =
            std::min(std::max(0.f, y00 - 0.344f * cb - 0.714f * cr), 255.f);
        row1[bIdx] = std::min(std::max(0.f, y00 + 1.402f * cr), 255.f);
      }
    }
  }
};
} // namespace icov::video
