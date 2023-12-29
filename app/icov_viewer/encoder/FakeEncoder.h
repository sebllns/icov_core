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
 *  @version 1.0.3
 *  @date 2023
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

#include "encoder/interface/IEncoder.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include <opencv2/opencv.hpp>

namespace icov {
namespace encoder {
class FakeEncoder : public IFrameEncoder<cv::Mat> {
  using IFrameEncoder<cv::Mat>::IFrameEncoder;

public:
  explicit FakeEncoder(std::ofstream &output);
  ~FakeEncoder();

private:
  cv::Mat m_frame;
  std::ofstream &m_output;

  // IFrameEncoder interface
protected:
  void processInput(const IInputMedia<cv::Mat> &input) override;
  void encodeBlockWithPrediction(
      const omnidirectional_layer::Block &b,
      omnidirectional_layer::IBlockPredictionManager::prediction_t prediction)
      override;
  std::vector<omnidirectional_layer::IBlockPredictionManager::prediction_t>
  getAvailablePredictions(const omnidirectional_layer::Block &b,
                          bool access_block,
                          const omnidirectional_layer::IBlockPredictionManager
                              &block_prediction) override;
};
} // namespace encoder
} // namespace icov
