/****************************************************************************/
/** @addtogroup video_layer
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup video_layer
 *  @ingroup icov-core
 */

/** @file BlockPrediction.h
 *  @brief BlockPrediction conveying BlockPrediction Units (VPU) for VCL data
 *  @ingroup common
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
#include <opencv2/core.hpp>

#include "ICOV_Picture.h"
#include "IcuEncoder.h"
#include "IntraPrediction.h"
#include "helpers.h"

namespace icov::video {

struct EncodingStructure {
  EncodingStructure();
  EncodingStructure(const std::vector<IcuEncoder> &icus, const UInt icusPerRow,
                    const UInt icusPerColumn);
  ~EncodingStructure();
  void set(const std::vector<IcuEncoder> &icus, const UInt icusPerRow,
           const UInt icusPerColumn);
  void setCheckPoints(const std::vector<Position> &checkPoints);
  void setCheckPoint(const Position &checkPoint);
  bool isAccessBlock(const Position &accessBlockPoint);

  std::vector<IcuEncoder> vec_icus;
  std::vector<Position> vec_checkPoints;
  UInt n_icusPerRow, n_icusPerCol;

  inline IcuEncoder &getICU(const UInt icu_index);

  inline IcuEncoder &getICU(const UInt x_index, const UInt y_index);

  inline const IcuEncoder &getICU(const UInt icu_index) const;

  inline bool isLeftVerticalBorder(const UInt icu_index) const;
  inline bool isRightVerticalBorder(const UInt icu_index) const;

  inline UInt get_icuIndex(const Position &pos) const;
  Position get_icuPosition(const UInt icu_index) const;

  inline void resetDecodedICUs();

  void updateRequestedICUs(
      const cv::Mat &maskRequestedPixels,
      std::vector<std::vector<Position>> &requestedICUs_closeToDecodedICUs,
      std::vector<std::vector<Position>> &requestedICUs_closeTocheckPoints,
      std::vector<Position> &neighborcheckPointInTheComponent,
      double *usefulness_percent = nullptr);

  void updateRequestedICUs(
      const cv::Mat &maskRequestedPixels,
      std::vector<std::vector<Position>> &requestedICUs_closeToDecodedICUs,
      std::vector<std::vector<Position>> &requestedICUs_closeTocheckPoints,
      std::vector<Position> &neighborcheckPointInTheComponent,
      std::vector<Position> &closestCheckPointToTheRemainingICUs,
      std::vector<std::vector<Position>>
          &remainingICUS_afterClosestCheckPointToBeDecoded,
      std::vector<std::vector<Position>>
          &remainingICUS_afterAlreadyProcessedICUsToBeDecoded);

  void reorderBlockDecodingBasedOnStartingAccessBlock(
      std::vector<std::vector<Position>> &in_out_requested_icus,
      const std::vector<Position> &accessBlockInsideAsStartingPosition);
  void get_decodedICUsInRequestedICUs(
      const cv::Mat &maskRequestedPixels,
      std::vector<std::vector<Position>> &requestedICUs_closeToDecodedICUs,
      std::vector<std::vector<Position>> &decodedICUs_inRequestedICUs);
  void updateDecodedICU(const Position &ICU_position);
  void updateDecodedICUs(
      const std::vector<std::vector<Position>> &connectedComponents);
  inline bool isICUAlreadyDecoded(const Position &ICU_position) const;
  inline const std::vector<Position> &get_vecDecodedICUs() const;

  inline const std::vector<Position> &get_vecCheckpoints() const;
  /** returns an integer which its last 4 least significant bits show which
   * sides are available. For more information refer to enum
   * REFERENCE_SAMPLE_SIDE Also the bit levels [5-8] (zero-based index) is 1 if
   * the corner point is also available: bit level 4 = (not used) to avoid
   * conflict of corner usage in REFERENCE_SAMPLE_SIDE. bit level 5 = top-left
   * corner bit level 6 = top-right corner bit level 7 = bottom-left corner bit
   * level 8 = bottom-right corner
   */
  int getAvailableDecodedSides_regularImage(const Position &icuPos) const;
  int getAvailableDecodedSides_360Image(const Position &icuPos) const;

  void create360VideoInteractiveCodingUnits_withoutAccessBlockComputation(
      ChromaFormat chFmt, const UInt sourceWidth_luma,
      const UInt sourceHeight_luma, const UInt sourceMargin_x_luma,
      const UInt icuWidth_luma, const UInt icuHeight_luma,
      const UInt tuWidth_luma, const UInt tuHeight_luma);

  void applyPredictionBasedOnEntropy(
      icov::video::Picture pic, icov::video::IntraPrediction &intraPred,
      const ComponentID compID, const int bitDepths[MAX_NUM_CHANNEL_TYPE],
      const UInt tuWidth_luma, const UInt tuHeight_luma,
      bool encodeFrequencyTableInBitStream, const int qp,
      int maxLog2TrDynamicRange,
      std::map<BlockPrediction::predType, cv::Mat> *oSiResult = nullptr);

protected:
  inline void resetAlreadyProcessedICUs();
  void updateAlreadyProccessedICU(const Position &ICU_position);
  inline void copyProccessedICUsFromDecodedICUs();
  inline bool isICUAlreadyProcessed(const Position &ICU_position);

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
