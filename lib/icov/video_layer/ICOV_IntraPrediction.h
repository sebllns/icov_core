/****************************************************************************/
/** @addtogroup Video
 *  Video contains the code related to video compression.
 *  @ingroup ICOV
 */

/** @file ICOV_IntraPrediction.h
 *  @brief ICOV_IntraPrediction
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
#include "CommonLib/IntraPrediction.h"
#include "ICOV_Storage.h"
#include "omnidirectional_layer/BlockPredictionManager.h"

namespace icov::video {

class IntraPrediction {
public:
    using REFERENCE_SAMPLE_SIDE = omnidirectional_layer::BlockPrediction::ePredType;

private:
  Pel *m_piYuvExt[MAX_NUM_COMPONENT][NUM_PRED_BUF];
  Int m_iYuvExtSize;

  static const UChar m_aucIntraFilter[MAX_NUM_CHANNEL_TYPE]
                                     [MAX_INTRA_FILTER_DEPTHS];

  CompArea m_lastArea;
  bool m_filterRefSamples;
  bool m_flipVert, m_flipHoriz;
  Int _bitDepth;

protected:
  ChromaFormat m_currChromaFormat;
  void destroy();
  void xFillReferenceSamples(const PelUnitBuf &recoUnitBuf,
                             const BoolUnitBuf &flagUnitBufReconstructedPixels,
                             Pel *refBufUnfiltered, const CompArea &area,
                             const Int bitDepths);
  void xFillReferenceSamples(const PelUnitBuf &recoUnitBuf,
                             const REFERENCE_SAMPLE_SIDE &refSampSide,
                             Pel *refBufUnfiltered, const CompArea &area,
                             const Int bitDepths);
  void xFillReferenceSamples(const PelUnitBuf &recoUnitBuf,
                             const BoolUnitBuf &flagUnitBufReconstructedPixels,
                             const REFERENCE_SAMPLE_SIDE &refSampSide,
                             Pel *refBufUnfiltered, const CompArea &area,
                             const Int bitDepths);

  void xFilterReferenceSamples(const Pel *refBufUnfiltered, Pel *refBufFiltered,
                               const CompArea &area);

  /// \brief xPredIntraPlanar
  ///  Function for deriving planar intra prediction. This function derives the
  /// prediction samples for planar mode (intra coding). NOTE: Bit-Limit -
  /// 24-bit source
  /// \param pSrc source
  /// \param pDst destination
  void xPredIntraPlanar(const CPelBuf &pSrc, PelBuf &pDst);

  Pel xGetPredValDc(
      const CPelBuf &pSrc,
      const Size &dstSize); // Function for calculating DC value of the
                            // reference samples used in Intra prediction. NOTE:
                            // Bit-Limit - 25-bit source
  inline void xPredIntraDc(const CPelBuf &pSrc, PelBuf &pDst) {
    pDst.fill(xGetPredValDc(pSrc, pDst));
  }

  /** Function for deriving the simplified angular intra predictions.
   *
   * This function derives the prediction samples for the angular mode based on
   * the prediction direction indicated by the prediction mode index. The
   * prediction direction is given by the displacement of the bottom row of the
   * block and the reference row above the block in the case of vertical
   * prediction or displacement of the rightmost column of the block and
   * reference column left from the block in the case of the horizontal
   * prediction. The displacement is signalled at 1/32 pixel accuracy. When
   * projection of the predicted pixel falls inbetween reference samples, the
   * predicted value for the pixel is linearly interpolated from the reference
   * samples. All reference samples are taken from the extended main reference.
   */
  void xPredIntraAng(const CPelBuf &pSrc, PelBuf &pDst, const UInt dirMode);

  Pel *getPredictorPtr(const ComponentID compID,
                       const Bool bUseFilteredPredictions = false) {
    return m_piYuvExt[compID][bUseFilteredPredictions ? PRED_BUF_FILTERED
                                                      : PRED_BUF_UNFILTERED];
  }

public:
  IntraPrediction();
  virtual ~IntraPrediction();

  void init(ChromaFormat chromaFormatIDC);

  /// set parameters from CU data for accessing intra data
  void initIntraPatternChType(const PelUnitBuf &recoUnitBuf,
                              const BoolUnitBuf &flagUnitBufReconstructedPixels,
                              const CompArea &area, const Int bitDepths,
                              const Bool bFilterRefSamples = false);
  void initIntraPatternChType(const PelUnitBuf &recoUnitBuf,
                              const REFERENCE_SAMPLE_SIDE &refSampSide,
                              const CompArea &area, const Int bitDepths,
                              const Bool bFilterRefSamples = false);
  void initIntraPatternChType(const PelUnitBuf &recoUnitBuf,
                              const BoolUnitBuf &flagUnitBufReconstructedPixels,
                              const REFERENCE_SAMPLE_SIDE &refSampSide,
                              const CompArea &area, const Int bitDepths,
                              const Bool bFilterRefSamples = false);

  /** returns an integer which its last 4 least significant bits shows which
   * sides are available. for more information refer to enum
   * REFERENCE_SAMPLE_SIDE Also the bit levels [5-8] (zero-based index) is 1 if
   * the corner point is also available: bit level 4 = (not used) to avoid
   * conflict of corner usage in REFERENCE_SAMPLE_SIDE. bit level 5 = top-left
   * corner bit level 6 = top-right corner bit level 7 = bottom-left corner bit
   * level 8 = bottom-right corner
   */
  int getAvailableSides(const BoolUnitBuf &flagUnitBufReconstructedPixels,
                        const CompArea &area);

  const CPelBuf getTopRowLeftColumnBuffer() const {
    return CPelBuf(m_piYuvExt[m_lastArea.compID][m_filterRefSamples],
                   m_lastArea.width + m_lastArea.height + 1,
                   m_lastArea.width + m_lastArea.height + 1,
                   m_lastArea.width + m_lastArea.height + 1);
  } // stride is equal to m_lastArea.width+m_lastArea.height+1. check end lines
    // of initIntraPatternChType for more information

  /// apply intra
  void predIntraAng(PelBuf &piPred, const UInt dirMode);
};

} // namespace icov::video
