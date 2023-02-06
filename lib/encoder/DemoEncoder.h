/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup Encoder
 *  ICOV Encoder Lib
 *  @ingroup ICOV
 */

/** @file DemoEncoder.h
 *  @brief Defines an actual ICOV DemoEncoder.
 *  @ingroup DemoEncoder
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
#include "encoder/interface/IEncoder.h"
#include "ldpc_codec_layer/IcovCoder.h"
#include "video_layer/Encodingstructure.h"

#include <map>
#include <opencv2/opencv.hpp>

namespace icov {
namespace encoder {
class DemoEncoder : public IFrameEncoder<cv::Mat> {
public:
  struct params {
    int qp;
    ChromaFormat chFormat;
    cv::Size icuSize_luma;
    cv::Size tuSize_luma;
    BitSymbolProvider::params bsp_params;
    po::variables_map program_args;
    bool enable_block_encoding;
    bool write_intra_pred_image;
    bool check_decoding;

    const Size tuSizeAfterRegrouping_luma = Size(1, 1);
    const int maxLog2TrDynamicRange = 15;

    /// in fact we need 1 margin for each color
    /// component, but since UnitArea
    /// handles division by 2 for chroma 420, it is easier to add 2 pixel
    /// margins per left and right side and this will be divided by 2 for
    /// chroma.
    const int margin_x_360luma = 2;

    /// Bit-depth the codec
    /// operates at (input/output files will be converted). (default:
    /// MSBExtendedBitDepth). If different to MSBExtendedBitDepth, source data
    /// will be converted
    const Int internalBitDepth[MAX_NUM_CHANNEL_TYPE] = {8, 8};
  };

  const static int PredViewID;
  const static int RecoViewID;
  const static int DecodedDebugViewID;

  DemoEncoder(int frame, const params &p);
  ~DemoEncoder();

  void onMouseViewPred(int event, int x, int y, int flags);
  void onMouseEncode(int event, int x, int y, int flags);

  void encode(std::ostream &output, IInputMedia<cv::Mat> &input,
              const IPicture &picture, const IAccessBlockManager &access_blocks,
              const IBlockPrediction &block_prediction) override;

protected:
  void encodeBlockWithPrediction(std::ostream &output, const Block &b,
                                 uint16_t prediction) override;
  void processInput(std::ostream &output,
                    const IInputMedia<cv::Mat> &input) override;

private:
  void onSelectBlock(int x, int y);
  void onEncodeBlock(int x, int y);

  void encode_icu(std::ostream &output, const Block &b,
                  const icov::video::IcuEncoder &icu_enc,
                  BlockPrediction::predType pred);
  void encode_si(std::ostream &output, const Block &b,
                 const icov::video::IcuEncoder &icu_enc,
                 const icov::video::SideInformationUnit &si,
                 BlockPrediction::predType pred, ComponentID comp);
  void encode_tu(std::ostream &output, const Block &b,
                 const icov::video::IcuEncoder &icu_enc,
                 const icov::video::SideInformationUnit &si,
                 BlockPrediction::predType pred, ComponentID comp,
                 int tu_index);

protected:
  params m_params;
  uint m_numComp;
  std::map<int, cv::Mat> m_cvmat;
  std::map<size_t, std::unique_ptr<AffInteractiveCoder>> m_coders;
  bool m_firstDisplayBlocks;
  icov::video::EncodingStructure m_enc_struc;
  cv::Point m_currentBlockPosition;
  icov::video::BufGroups m_vvc_bufs;
  const IPicture *m_pic_ptr;
};
} // namespace encoder
} // namespace icov
