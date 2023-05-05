/****************************************************************************/
/** @addtogroup Video
 *  Video contains the code related to video compression.
 *  @ingroup ICOV
 */

/** @file ICOV_DistorsionCost.h
 *  @brief ICOV_DistorsionCost
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
#include "CommonLib/RdCost.h"

namespace icov::video {

class DistortionCost {
private:
  static FpDistFunc m_afpDistortFunc[DF_TOTAL_FUNCTIONS]; // [eDFunc]
  double m_distortionWeight[MAX_NUM_COMPONENT]; // only chroma values are used.

public:
  DistortionCost();
  virtual ~DistortionCost();
  Distortion getDistPart(const CPelBuf &org, const CPelBuf &cur, Int bitDepth,
                         const ComponentID compID, DFunc eDFunc);

  Distortion getDistPart(const CCoeffBuf &org, const CCoeffBuf &cur,
                         Int bitDepth, const ComponentID compID, DFunc eDFunc);

  void setDistortionChromaWeight(const ComponentID compID,
                                 const Double distortionWeight) {
    m_distortionWeight[compID] = distortionWeight;
  }
  void setDistortionChromaWeight(const ComponentID compID, const Int qpy,
                                 const ChromaFormat chFmt,
                                 const Int chromaQPOffset);

private:
  void init();
  void initRdCostX86();

  static Distortion xGetSSEw(const DistParam &rcDtParam); // weighted SSD cost
  static Distortion xGetSSE(const DistParam &rcDtParam);
  static Distortion xGetSSE4(const DistParam &rcDtParam);
  static Distortion xGetSSE8(const DistParam &rcDtParam);
  static Distortion xGetSSE16(const DistParam &rcDtParam);
  static Distortion xGetSSE16N(const DistParam &rcDtParam);
  static Distortion xGetSSE32(const DistParam &rcDtParam);
  static Distortion xGetSSE64(const DistParam &rcDtParam);

  static Distortion xGetSAD(const DistParam &rcDtParam);
  static Distortion xGetSAD4(const DistParam &rcDtParam);
  static Distortion xGetSAD8(const DistParam &rcDtParam);
  static Distortion xGetSAD12(const DistParam &rcDtParam);
  static Distortion xGetSAD16(const DistParam &rcDtParam);
  static Distortion xGetSAD32(const DistParam &rcDtParam);
  static Distortion xGetSAD64(const DistParam &rcDtParam);
  static Distortion xGetSAD16N(const DistParam &rcDtParam);

  static Distortion xGetSAD24(const DistParam &rcDtParam);
  static Distortion xGetSAD48(const DistParam &rcDtParam);
};
} // namespace icov::video
