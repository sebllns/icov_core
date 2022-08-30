/****************************************************************************/
/** @addtogroup video_layer
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup video_layer
 *  @ingroup icov-core
 */

/** @file ICOV_Units.h
 *  @brief ICOV_Units
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
#include <memory>

#include "CommonLib/Unit.h"
#include "ICOV_IntraPrediction.h"
#include "ICOV_Storage.h"
#include "common.h"

namespace icov::video {

struct TransformUnit : public UnitArea {
public:
  TransformUnit(const UnitArea &unit, bool createTransformBuffer,
                bool createQuantizedTransformBuffer,
                bool createInversetQuantizedTransformBuffer);
  TransformUnit(const ChromaFormat chromaFormat, const Area &lumaArea,
                bool createTransformBuffer, bool createQuantizedTransformBuffer,
                bool createInversetQuantizedTransformBuffer);
  ~TransformUnit();

  //    std::shared_ptr<TransformUnit> next, previous;
  TransformUnit *next, *previous;

  CoeffBuf &getTransformCoeffs(const ComponentID id) {
    return _Tcoeffs.getBuf(id);
  }

  const CCoeffBuf getTransformCoeffs(const ComponentID id) const {
    return _Tcoeffs.getBuf(id);
  }

  CoeffBuf &getQuantizedTransformCoeffs(const ComponentID id) {
    return _quantizedTcoeffs.getBuf(id);
  }
  const CCoeffBuf getQuantizedTransformCoeffs(const ComponentID id) const {
    return _quantizedTcoeffs.getBuf(id);
  }

  CoeffBuf &getInverseQuantizedTransformCoeffs(const ComponentID id) {
    return _inverseQuantizedTcoeffs.getBuf(id);
  }

  const CCoeffBuf
  getInverseQuantizedTransformCoeffs(const ComponentID id) const {
    return _inverseQuantizedTcoeffs.getBuf(id);
  }

  icov::Bitplanes &getBitplanes(const ComponentID id) {
    return m_bitplanes[id];
  }

  const icov::Bitplanes getBitplanes(const ComponentID id) const {
    return m_bitplanes[id];
  }

  const IQTCoeffStorage getInverseQuantizedTcoeffs() const {
    return _inverseQuantizedTcoeffs;
  }

  const IQTCoeffStorage getQuantizedTcoeffs() const {
    return _quantizedTcoeffs;
  }

protected:
  TCoeffStorage _Tcoeffs;
  QTCoeffStorage _quantizedTcoeffs;
  IQTCoeffStorage _inverseQuantizedTcoeffs;
  icov::Bitplanes m_bitplanes[MAX_NUM_COMPONENT];
};

struct InteractiveCodingUnit : public UnitArea {
  InteractiveCodingUnit(const UnitArea &unit, bool createTransformBuffer = true,
                        bool createQuantizedTransformBuffer = true,
                        bool createInverseQuantizedTransformBuffer = true);
  InteractiveCodingUnit(const ChromaFormat chromaFormat, const Area &lumaArea,
                        bool createTransformBuffer = true,
                        bool createQuantizedTransformBuffer = true,
                        bool createInverseQuantizedTransformBuffer = true);

  std::vector<TransformUnit> TUs;

  TransformUnit *addTU(const UnitArea &unit);
  TransformUnit *getTU(const UnitArea &unit);

  bool needTransformBuffer() { return _needTransformBuffer; }
  bool needQuantizedTransformBuffer() { return _needQuantizedTransformBuffer; }
  bool needInverseQuantizedTransformBuffer() {
    return _needInverseQuantizedTransformBuffer;
  }

protected:
  bool _needTransformBuffer, _needQuantizedTransformBuffer,
      _needInverseQuantizedTransformBuffer;
};

struct SideInformationUnit : public InteractiveCodingUnit {
  SideInformationUnit(const UnitArea &unit, bool createTransformBuffer = true,
                      bool createQuantizedTransformBuffer = false,
                      bool createInverseQuantizedTransformBuffer = false)
      : InteractiveCodingUnit(unit, createTransformBuffer,
                              createQuantizedTransformBuffer,
                              createInverseQuantizedTransformBuffer) {}

  SideInformationUnit(const ChromaFormat chromaFormat, const Area &lumaArea,
                      bool createTransformBuffer = true,
                      bool createQuantizedTransformBuffer = false,
                      bool createInverseQuantizedTransformBuffer = false)
      : InteractiveCodingUnit(chromaFormat, lumaArea, createTransformBuffer,
                              createQuantizedTransformBuffer,
                              createInverseQuantizedTransformBuffer) {}

  UChar intraDir[MAX_NUM_COMPONENT]; // intra prediction direction mode
  std::map<UInt, UInt>
      bitsToDecode[MAX_NUM_COMPONENT]; // bits needed for decoding per band.
                                       // key
                                       // represents band index and value
                                       // represents number of bits

  int estimateSIRequiredBitsToCodeIntraMode(
      const ComponentID compID, UInt dirMode,
      const icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE &refSampSide)
      const;
};

} // namespace icov::video
