/****************************************************************************/
/** @addtogroup Video
 *  Video contains the code related to video compression.
 *  @ingroup ICOV
 */

/** @file IcuEncoder.h
 *  @brief IcuEncoder
 *  @ingroup Video
 *  @author Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.0.0
 *  @date 2018, 2022
 *  @copyright Copyright (C) 2022 by Inria. All rights reserved.
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
#include "ICOV_Units.h"

namespace icov::video {
class IcuEncoder {
protected:
  InteractiveCodingUnit _sourceX;
  std::map<icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE,
           SideInformationUnit>
      _mapIntraSIs;

public:
  IcuEncoder(const UnitArea &unit, bool createTransformBuffer = true,
             bool createQuantizedTransformBuffer = true,
             bool createInverseQuantizedTransformBuffer = true)
      : _sourceX(unit, createTransformBuffer, createQuantizedTransformBuffer,
                 createInverseQuantizedTransformBuffer) {}
  IcuEncoder(const ChromaFormat chromaFormat, const Area &lumaArea,
             bool createTransformBuffer = true,
             bool createQuantizedTransformBuffer = true,
             bool createInverseQuantizedTransformBuffer = true)
      : _sourceX(chromaFormat, lumaArea, createTransformBuffer,
                 createQuantizedTransformBuffer,
                 createInverseQuantizedTransformBuffer) {}

  bool
  addIntraSI(icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE refSampSide,
             bool createTransformBuffer = true,
             bool createQuantizedTransformBuffer = false,
             bool createInverseQuantizedTransformBuffer = false);

  inline InteractiveCodingUnit &get_sourceX() { return _sourceX; }
  inline const InteractiveCodingUnit &get_sourceX() const { return _sourceX; }
  inline std::vector<TransformUnit> &sourceX_TUs() { return _sourceX.TUs; }
  inline const std::vector<TransformUnit> &sourceX_TUs() const {
    return _sourceX.TUs;
  }

  inline TransformUnit &sourceX_TU(int i) { return _sourceX.TUs[i]; }
  inline const TransformUnit &sourceX_TU(int i) const {
    return _sourceX.TUs[i];
  }

  void createTUs(const UInt tuWidth_luma, const UInt tuHeight_luma);

  SideInformationUnit *get_sideInformation(
      const icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE &rfSamp);
  const SideInformationUnit *get_sideInformation(
      const icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE &rfSamp) const;

  //    void calcConditionalEntropy(const ComponentID compId, const
  //    REFERENCE_SAMPLE_SIDE rfSamp, const UInt bandIndex, const UInt tuWidth,
  //    const UInt tuHeight,
  //                               std::vector<Position>&
  //                               sequenceTransfromValues,
  //                               std::vector<Position>& uniqueTransformValues,
  //                               std::vector<double>& probs_XY);

  UInt calcDistributions(
      const ComponentID compId,
      const icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE rfSamp,
      const UInt bandIndex, const UInt tuWidth, const UInt tuHeight,
      std::vector<TCoeff> &transformSequence_sourceX,
      std::vector<TCoeff> &transformSequence_sourceY,
      std::map<TCoeff, std::map<TCoeff, double>> &probs_XY,
      std::map<TCoeff, std::map<TCoeff, double>>
          &probs_YX, // probs_YX is same is probs_XY but not the first element
                     // is y
      std::map<TCoeff, double> &probs_X,
      std::map<TCoeff, double>
          &probs_Y); // function returns conditional entropy of XgivenY times
                     // length of the sequence: sequenceLength * H(X|Y)

  void getTransformSequences(
      const ComponentID compId,
      const icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE rfSamp,
      const UInt bandIndex, const UInt tuWidth, const UInt tuHeight,
      std::vector<TCoeff> &transformSequence_sourceX,
      std::vector<TCoeff> &transformSequence_sourceY) const;

  UInt calcDistributionsBasedOnResidual(
      const ComponentID compId,
      const icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE rfSamp,
      const UInt bandIndex, const UInt tuWidth, const UInt tuHeight,
      std::vector<TCoeff> &transformSequence_sourceX,
      std::vector<TCoeff> &transformSequence_sourceY,
      std::vector<int> &residual, std::map<int, double> &probs_residualXminusY,
      bool compressBasedOnAbsValues, int maxLog2DynamicRange);

  void calcDistributions(
      const ComponentID compId, std::vector<UInt> &numberOfDecodingBits,
      const UInt bandIndex, const UInt tuWidth, const UInt tuHeight,
      std::vector<std::vector<TCoeff>> &transformSequence_sourceX,
      std::vector<std::vector<TCoeff>> &allSIs_transformSequence_sourceY,
      std::vector<std::map<TCoeff, std::map<TCoeff, double>>> &allSIs_probs_XY,
      std::vector<std::map<TCoeff, std::map<TCoeff, double>>>
          &allSIs_probs_YX, // probs_YX is same is probs_XY but not the first
                            // element is y
      std::vector<std::map<TCoeff, double>> &allSIs_probs_X,
      std::vector<std::map<TCoeff, double>>
          &allSIs_probs_Y); // function returns conditional entropy of XgivenY
                            // times sequence length

  void calcDistributionsBasedOnResidual(
      const ComponentID compId, std::vector<UInt> &numberOfDecodingBits,
      const UInt bandIndex, const UInt tuWidth, const UInt tuHeight,
      std::vector<std::vector<TCoeff>> &transformSequence_sourceX,
      std::vector<std::vector<TCoeff>> &allSIs_transformSequence_sourceY,
      std::vector<std::vector<int>> &vec_residual,
      std::vector<std::map<int, double>> &vec_probs_residualXminusY,
      bool compressBasedOnAbsValues, int maxLog2DynamicRange);

  std::map<icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE,
           SideInformationUnit> &
  get_intraSIsMap() {
    return _mapIntraSIs;
  }
  const std::map<icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE,
                 SideInformationUnit> &
  get_intraSIsMap() const {
    return _mapIntraSIs;
  }
  inline UInt numberOfSIs() const {
    return _mapIntraSIs.size();
  } // TODO: don't forget to change it when interprediction is also added

protected:
  bool addTU(const UnitArea &unit);
};

} // namespace icov::video
