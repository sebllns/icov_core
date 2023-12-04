/****************************************************************************/
/** @addtogroup Video
 *  Video contains the code related to video compression.
 *  @ingroup ICOV
 */

/** @file EncodingStructure.h
 *  @brief EncodingStructure
 *  @ingroup common
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
#include <opencv2/core.hpp>

#include "ICOV_BufGroups.h"
#include "IcuEncoder.h"
#include "omnidirectional_layer/Block.h"
namespace icov::video {

struct EncodingStructure {
public:
  EncodingStructure();
  EncodingStructure(const std::vector<IcuEncoder> &icus, const UInt icusPerRow,
                    const UInt icusPerColumn);
  ~EncodingStructure();
  void set(const std::vector<IcuEncoder> &icus, const UInt icusPerRow,
           const UInt icusPerColumn);

  IcuEncoder &getICU(const UInt icu_index);
  const IcuEncoder &getICU(const UInt icu_index) const;

  IcuEncoder &getICU(const UInt x_index, const UInt y_index);

  bool isLeftVerticalBorder(const UInt icu_index) const;
  bool isRightVerticalBorder(const UInt icu_index) const;

  UInt get_icuIndex(const Position &pos) const;
  Position get_icuPosition(const UInt icu_index) const;
  UInt get_icuIndex(const omnidirectional_layer::Block &b) const;

  void create360VideoInteractiveCodingUnits_withoutAccessBlockComputation(
      ChromaFormat chFmt, const UInt sourceWidth_luma,
      const UInt sourceHeight_luma, const UInt sourceMargin_x_luma,
      const UInt icuWidth_luma, const UInt icuHeight_luma,
      const UInt tuWidth_luma, const UInt tuHeight_luma);

  void applyPredictionBasedOnEntropy(icov::video::BufGroups &encpic,
                                     const ComponentID compID,
                                     const int bitDepths[MAX_NUM_CHANNEL_TYPE],
                                     const UInt tuWidth_luma,
                                     const UInt tuHeight_luma,
                                     bool encodeFrequencyTableInBitStream,
                                     const int qp, int maxLog2TrDynamicRange);

  const std::vector<IcuEncoder> &vec_icus() const;
  UInt n_icusPerRow() const;
  UInt n_icusPerCol() const;
  size_t n_icus() const;

protected:
  std::vector<IcuEncoder> _vec_icus;
  std::vector<Position> _vec_checkPoints;
  UInt _n_icusPerRow, _n_icusPerCol;

  cv::Mat _integralImageMask;
  cv::Mat _maskThresholded;
  std::vector<int> _vec_pixelsRequestedPerICU;
  std::vector<Position> _vec_decodedICUs;
  AreaBuf<bool> _requestedICUs, _decodedICUs;
  std::vector<Position> _vec_tmp_alreadyProcessedICUs;
  AreaBuf<bool> _tmp_alreadyProcessedICUs;
  void deallocate();
};
} // namespace icov::video
