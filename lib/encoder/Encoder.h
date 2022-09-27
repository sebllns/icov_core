/****************************************************************************/
/** @addtogroup icov-core
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup Encoder
 * Encoder package (client side).
 *  @ingroup icov-core
 */

/** @file Encoder.h
 *  @brief Defines an actual ICOV Encoder.
 *  @ingroup Encoder
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
#include "omnidirectional_layer/interface/IEncoder.h"
#include "video_layer/Encodingstructure.h"

#include <map>
#include <opencv2/opencv.hpp>

namespace icov {
class Encoder : public IEncoder {
protected:
  std::map<int, cv::Mat> m_cvmat;
  bool m_firstDisplayBlocks;

  cv::Size m_icuSize_luma;
  cv::Size m_tuSize_luma;
  int m_qp;
  icov::video::EncodingStructure m_enc_struc;
  cv::Point m_currentBlockPosition;
  ChromaFormat m_chromaFormatIDC;
  uint m_numComp;
  icov::video::BufGroups vvc_pic;

public:
  const static int BlockZoomID;
  const static int RecoViewID;

  Encoder();
  ~Encoder();

  // IEncoder interface
public:
  void openFile(const std::string &filename) override;
  void dispose() override;
  void encode(std::ostream &output, const IPicture &picture,
              const IAccessBlockManager &access_blocks) override;
  void encodeBlock(std::ostream &output, const Block &b,
                   bool access_block) override;
  void encodeBlockWithPrediction(std::ostream &output, const Block &b,
                                 uint16_t prediction) override;
  void onMouse(int event, int x, int y, int flags);

private:
  void onSelectBlock(int x, int y);
};
} // namespace icov
