/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup Encoder
 *  ICOV Encoder Lib
 *  @ingroup ICOV
 */

/** @file encoder.h
 *  @brief Defines an actual ICOV encoder.
 *  @ingroup encoder
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.0.1
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

#include "encoder/interface/IEncoder.h"
#include <opencv2/opencv.hpp>

namespace icov {
namespace encoder {
class FakeEncoder : public IFrameEncoder<cv::Mat> {
  using IFrameEncoder<cv::Mat>::IFrameEncoder;

public:
  ~FakeEncoder();

  // IEncoder interface
protected:
  cv::Mat m_frame;

  void processInput(std::ostream &output,
                    const IInputMedia<cv::Mat> &input) override;

  // IFrameEncoder interface
protected:
  void encodeBlockWithPrediction(std::ostream &output, const Block &b,
                                 uint16_t prediction) override;
  std::vector<uint16_t>
  getAvailablePredictions(const Block &b, bool access_block,
                          const IBlockPrediction &block_prediction) override;
};
} // namespace encoder
} // namespace icov
