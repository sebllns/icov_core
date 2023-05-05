/****************************************************************************/
/** @addtogroup Video
 *  Video contains the code related to video compression.
 *  @ingroup ICOV
 */

/** @file ICOV_Quant.h
 *  @brief ICOV_Quant
 *  @ingroup Video
 *  @author Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.0.0
 *  @date 2018, 2022
 *  @copyright Copyright (C) 2023 by Inria. All rights reserved.
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
#include "CommonLib/Buffer.h"
#include "CommonLib/CommonDef.h"
#include <map>

namespace icov::video {

/// QP struct
struct QpParam {
  Int Qp;
  Int per;
  Int rem;

  ChannelType channelType;
  ChromaFormat chromaFomat;

  QpParam(const Int qpy, const ChannelType chType, const ChromaFormat chFmt,
          const Int channelBitDepth, const Int chromaQPOffset = 0,
          const int dqp = 0);

}; // END STRUCT DEFINITION QpParam

// struct Key_QuantizationMatrix : public Size
//{
//    Key_QuantizationMatrix(const SizeType Width, const SizeType Height, const
//    Int qp) : Size(Width, Height), qp_rem(qp) {} Bool operator==(const
//    Key_QuantizationMatrix &other) const { return (Size::operator==(other)) &&
//    (qp_rem==other.qp_rem); } Bool operator==(const Key_QuantizationMatrix
//    &other) const { return !(*this==other);}

//    Int qp_rem;
//};

class Quant {
protected:
  struct WeightingMatrixComparator {

    bool operator()(const std::pair<Size, Int> &left,
                    const std::pair<Size, Int> &right) const {
      return left.first.width < right.first.width ||
             (!(right.first.width < left.first.width) &&
              left.first.height < right.first.height) ||
             (!(right.first.width < left.first.width) &&
              !(right.first.height < left.first.height) &&
              left.second < right.second);
    }
  };

  std::map<std::pair<Size, Int>, AreaBuf<Int>, WeightingMatrixComparator>
      _mapQuantizerWeightingMatrix; // quantization matrix for a specific size
                                    // (UInt width, UInt height) and qp_rem
                                    // (remainder (q)
  std::map<std::pair<Size, Int>, AreaBuf<Int>, WeightingMatrixComparator>
      _mapDequantizerWeightingMatrix; // quantization matrix for a specific size
                                      // (UInt width, UInt height) and qp_rem
                                      // (remainder (q)
public:
  Quant();
  ~Quant();
  const AreaBuf<const Int>
  getDefaultQuantizationMatrix(const Size &size, const QpParam &cQP,
                               const SliceType sliceType);
  const AreaBuf<const Int>
  getDefaultDequantizationMatrix(const Size &size, const QpParam &cQP,
                                 const SliceType sliceType);
  // quantization
  virtual void quant(CoeffBuf &piQCoef, const CCoeffBuf &pSrc, TCoeff &uiAbsSum,
                     const QpParam &cQP, const SliceType sliceType,
                     const Int channelBitDepth,
                     const int maxLog2TrDynamicRange = 15);
  virtual void dequant(CoeffBuf &piCoef, const CCoeffBuf &piQCoef,
                       const QpParam &cQP, const Int channelBitDepth,
                       const int maxLog2TrDynamicRange = 15);
};

} // namespace icov::video
