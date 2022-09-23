/****************************************************************************/
/** @addtogroup video_layer
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup video_layer
 *  @ingroup icov-core
 */

/** @file video_common.h
 *  @brief video_common
 *  @ingroup video_layer
 *  @author Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
 *  @author S�bastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
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
#include "CommonLib/Unit.h"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace VTM_OPENCV_Utility {

double mse(const cv::Mat &I1, const cv::Mat &I2);
double psnr(const cv::Mat &I1, const cv::Mat &I2);

template <class DataType>
cv::Mat_<DataType> toMat(const AreaBuf<const DataType> &areaBuffer) {
  size_t step_Y = areaBuffer.stride *
                  sizeof(DataType); // pel is 16-bit (2 bytes) and step is
                                    // number of BYTES each matrix row occupies
  int width_Y = areaBuffer.width;
  int height_Y = areaBuffer.height;
  const DataType *ptr_Y = areaBuffer.bufAt(0, 0);

  return cv::Mat_<DataType>(
      height_Y, width_Y, const_cast<DataType *>(ptr_Y),
      step_Y); // This way no data is copied. Pel is of short type (i.e. 16 bit)
               // so we should use CV_16SC1
}

template <class DataType>
cv::Mat_<DataType> toMat(const UnitBuf<const DataType> &unitBuffer,
                         const ComponentID comp) {
  const AreaBuf<const DataType> &areaBuff = unitBuffer.get(comp);
  return toMat<DataType>(areaBuff);
}

inline cv::Mat_<short> toMat_16S(const AreaBuf<const Pel> &areaBuffer) {
  return toMat<short>(areaBuffer);
} // this is the only way to handle different function names for different
  // types. any help will be appreciated.
inline cv::Mat_<short> toMat_16S(const UnitBuf<const Pel> &unitBuffer,
                                 const ComponentID comp) {
  return toMat<short>(unitBuffer, comp);
} // this is the only way to handle different function names for different
  // types. any help will be appreciated.

void cvMatToYUV(const cv::Mat &input, PelUnitBuf &pic);
void toBGRMat(const PelUnitBuf &src, cv::Mat &output);

template <class DataType>
double calculateSSIM(const AreaBuf<const DataType> &i1,
                     const AreaBuf<const DataType> &i2, Int bitdepth) {
<<<<<<< HEAD

  const float lum_range = float((1 << bitdepth) - 1);
  //    std::cout << "lum_range= " << lum_range << std::endl;
  const float k1 = 0.01f, k2 = 0.03f;
  const float C1 = std::pow(k1 * lum_range, 2.f),
              C2 = std::pow(k2 * lum_range, 2.f);
  //    std::cout << "C1= " << C1 << ", C2= " << C2 << std::endl;

  /***************************** INITS **********************************/
  int d = CV_32FC1;
  cv::Mat I1, I2;
  toMat(i1).convertTo(I1, d); // cannot calculate on one byte large values
  toMat(i2).convertTo(I2, d);

  cv::Mat I2_2 = I2.mul(I2);  // I2^2
  cv::Mat I1_2 = I1.mul(I1);  // I1^2
  cv::Mat I1_I2 = I1.mul(I2); // I1 * I2

  /*************************** END INITS **********************************/

  cv::Mat mu1, mu2; // PRELIMINARY COMPUTING
  cv::GaussianBlur(I1, mu1, cv::Size(11, 11), 1.5);
  cv::GaussianBlur(I2, mu2, cv::Size(11, 11), 1.5);

  cv::Mat mu1_2 = mu1.mul(mu1);
  cv::Mat mu2_2 = mu2.mul(mu2);
  cv::Mat mu1_mu2 = mu1.mul(mu2);

  cv::Mat sigma1_2, sigma2_2, sigma12;

  cv::GaussianBlur(I1_2, sigma1_2, cv::Size(11, 11), 1.5);
  sigma1_2 -= mu1_2;

  cv::GaussianBlur(I2_2, sigma2_2, cv::Size(11, 11), 1.5);
  sigma2_2 -= mu2_2;

  cv::GaussianBlur(I1_I2, sigma12, cv::Size(11, 11), 1.5);
  sigma12 -= mu1_mu2;

  ///////////////////////////////// FORMULA ////////////////////////////////

  cv::Mat t1, t2, t3;

  t1 = 2 * mu1_mu2 + C1;
  t2 = 2 * sigma12 + C2;
  t3 = t1.mul(t2); // t3 = ((2*mu1_mu2 + C1).*(2*sigma12 + C2))

  t1 = mu1_2 + mu2_2 + C1;
  t2 = sigma1_2 + sigma2_2 + C2;
  t1 = t1.mul(t2); // t1 =((mu1_2 + mu2_2 + C1).*(sigma1_2 + sigma2_2 + C2))

  cv::Mat ssim_map;
  cv::divide(t3, t1, ssim_map); // ssim_map =  t3./t1;

=======

  const float lum_range = float((1 << bitdepth) - 1);
  //    std::cout << "lum_range= " << lum_range << std::endl;
  const float k1 = 0.01f, k2 = 0.03f;
  const float C1 = std::pow(k1 * lum_range, 2.f),
              C2 = std::pow(k2 * lum_range, 2.f);
  //    std::cout << "C1= " << C1 << ", C2= " << C2 << std::endl;

  /***************************** INITS **********************************/
  int d = CV_32FC1;
  cv::Mat I1, I2;
  toMat(i1).convertTo(I1, d); // cannot calculate on one byte large values
  toMat(i2).convertTo(I2, d);

  cv::Mat I2_2 = I2.mul(I2);  // I2^2
  cv::Mat I1_2 = I1.mul(I1);  // I1^2
  cv::Mat I1_I2 = I1.mul(I2); // I1 * I2

  /*************************** END INITS **********************************/

  cv::Mat mu1, mu2; // PRELIMINARY COMPUTING
  cv::GaussianBlur(I1, mu1, cv::Size(11, 11), 1.5);
  cv::GaussianBlur(I2, mu2, cv::Size(11, 11), 1.5);

  cv::Mat mu1_2 = mu1.mul(mu1);
  cv::Mat mu2_2 = mu2.mul(mu2);
  cv::Mat mu1_mu2 = mu1.mul(mu2);

  cv::Mat sigma1_2, sigma2_2, sigma12;

  cv::GaussianBlur(I1_2, sigma1_2, cv::Size(11, 11), 1.5);
  sigma1_2 -= mu1_2;

  cv::GaussianBlur(I2_2, sigma2_2, cv::Size(11, 11), 1.5);
  sigma2_2 -= mu2_2;

  cv::GaussianBlur(I1_I2, sigma12, cv::Size(11, 11), 1.5);
  sigma12 -= mu1_mu2;

  ///////////////////////////////// FORMULA ////////////////////////////////

  cv::Mat t1, t2, t3;

  t1 = 2 * mu1_mu2 + C1;
  t2 = 2 * sigma12 + C2;
  t3 = t1.mul(t2); // t3 = ((2*mu1_mu2 + C1).*(2*sigma12 + C2))

  t1 = mu1_2 + mu2_2 + C1;
  t2 = sigma1_2 + sigma2_2 + C2;
  t1 = t1.mul(t2); // t1 =((mu1_2 + mu2_2 + C1).*(sigma1_2 + sigma2_2 + C2))

  cv::Mat ssim_map;
  cv::divide(t3, t1, ssim_map); // ssim_map =  t3./t1;

>>>>>>> dev-si
  cv::Scalar mssim = mean(ssim_map); // mssim = average of ssim map
  //    std::cout << "mssim= "<< mssim << std::endl;
  return mssim[0];
}
} // namespace VTM_OPENCV_Utility
