/****************************************************************************/
/** @addtogroup icov-core
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup FrameEncoder
 * FrameEncoder package (client side).
 *  @ingroup icov-core
 */

/** @file FrameEncoder.h
 *  @brief Defines an actual ICOV FrameEncoder.
 *  @ingroup FrameEncoder
 *  @author S�bastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 0.0.1
 *  @date 2022
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
class FrameEncoder : public IFrameEncoder<cv::Mat> {
public:
  struct params {
    int qp;
    ChromaFormat chFormat;
    cv::Size icuSize_luma;
    cv::Size tuSize_luma;
    BitSymbolProvider::params bsp_params;
  };

  const static int BlockZoomID;
  const static int RecoViewID;

  FrameEncoder(int frame, const params &p);
  ~FrameEncoder();

  // IFrameEncoder interface
  void onMouse(int event, int x, int y, int flags);

protected:
  void encodeBlockWithPrediction(std::ostream &output, const Block &b,
                                 uint16_t prediction) override;
  void processInput(std::ostream &output,
                    const IMedia<cv::Mat> &input) override;

private:
  void onSelectBlock(int x, int y);

  void encode_si(const icov::video::IcuEncoder &icu_enc,
                 BlockPrediction::predType pred) const;

protected:
  params m_params;
  uint m_numComp;
  std::map<int, cv::Mat> m_cvmat;
  std::map<int, std::unique_ptr<AffInteractiveCoder>> m_coders;
  bool m_firstDisplayBlocks;
  icov::video::EncodingStructure m_enc_struc;
  cv::Point m_currentBlockPosition;
  icov::video::BufGroups m_vvc_bufs;
};
} // namespace encoder
} // namespace icov
