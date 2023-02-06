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

#include "BlockPrediction.h"
#include "Media.h"
#include "ldpc_codec_layer/IcovCoder.h"
#include "omnidirectional_layer/interface/IBlockDecoder.h"
#include "video_layer/Encodingstructure.h"

#include <map>
#include <opencv2/opencv.hpp>

namespace icov {
namespace decoder {
class BlockDecoder : public IBlockDecoder {
  // IBlockProcessor interface
public:
  struct params {
    int qp;
    ChromaFormat chFormat;

    // TODO: unplug openCV from DECODER
    cv::Size sourceSize;
    cv::Size icuSize_luma;
    cv::Size tuSize_luma;

    BitSymbolProvider::params bsp_params;
    po::variables_map program_args;
    bool enable_block_encoding;
    bool write_intra_pred_image;

    const Size tuSizeAfterRegrouping_luma = Size(1, 1);
    const int maxLog2TrDynamicRange = 15;
    const int margin_x_360luma = 2; // in fact we need 1 margin for each color
                                    // component, but since UnitArea
    // handles division by 2 for chroma 420, it is easier to add 2 pixel
    // margins per left and right side and this will be divided by 2 for
    // chroma.

    const Int internalBitDepth[MAX_NUM_CHANNEL_TYPE] = {8, 8};
  };

  BlockDecoder(const BlockDecoder::params &p);
  ~BlockDecoder();

  void startProcess() override;
  void processBlock(const Block &b, uint16_t prediction) override;
  void endProcess() override;
  void decode() override;

protected:
  void decode_icu(const Block &b, const icov::video::IcuEncoder &icu_enc,
                  BlockPrediction::predType pred);

  params m_params;
  icov::video::EncodingStructure m_dec_struc;
};
} // namespace decoder
} // namespace icov
