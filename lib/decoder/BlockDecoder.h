/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup BlockDecoder
 * BlockDecoder package (client side).
 *  @ingroup ICOV
 */

/** @file BlockDecoder.h
 *  @brief Defines an actual ICOV BlockDecoder.
 *  @ingroup BlockDecoder
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.0.3
 *  @date 2023
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

#include "decoder/ChannelDecoder.h"
#include "ldpc_codec_layer/BitSymbolProvider.h"
#include "ldpc_codec_layer/factory/LdpcCoderParameters.h"

#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "omnidirectional_layer/interface/IBlockDecoder.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include "video_layer/Encodingstructure.h"
#include "video_layer/extracted_stream.h"

#include <boost/type_traits/extent.hpp>
#include <map>
#include <memory>
#include <opencv2/opencv.hpp>

namespace icov {
namespace decoder {
class BlockDecoder : public omnidirectional_layer::IBlockDecoder {
  // IBlockProcessor interface
public:
  struct params {
    int qp;
    ChromaFormat chFormat;

    // TODO: unplug openCV from DECODER
    cv::Size sourceSize;
    cv::Size icuSize_luma;

    ldpc_codec_layer::BitSymbolProvider::params bsp_params;

    //    const Size tuSizeAfterRegrouping_luma = Size(1, 1);
    const int maxLog2TrDynamicRange = 15;
    const int margin_x_360luma = 2; // in fact we need 1 margin for each color
                                    // component, but since UnitArea
    // handles division by 2 for chroma 420, it is easier to add 2 pixel
    // margins per left and right side and this will be divided by 2 for
    // chroma.

    const Int internalBitDepth[MAX_NUM_CHANNEL_TYPE] = {8, 8};
  };

  BlockDecoder(const BlockDecoder::params &p,
               const LdpcCoderParameters &coder_params,
               const bitstream::sp_IcovExtractedBitstream &extraction);
  ~BlockDecoder();

  void startProcess() override;
  void processBlock(const omnidirectional_layer::Block &b,
                    omnidirectional_layer::IBlockPredictionManager::prediction_t
                        prediction) override;
  void endProcess() override;
  void decode() override;
  const PelUnitBuf decodingOutput() const;

protected:
  void decode_icu(const omnidirectional_layer::Block &b,
                  const icov::video::IcuEncoder &icu_enc,
                    omnidirectional_layer::BlockPrediction::ePredType pred);

  std::shared_ptr<icov::video::EncodingStructure> m_dec_struc;
  std::shared_ptr<icov::video::BufGroups> m_vvc_bufs;
  int m_channelCount;
  std::vector<std::unique_ptr<ChannelDecoder>> m_channelDecoder;
  params m_params;
  UnitArea mainFrameArea;
};
} // namespace decoder
} // namespace icov
