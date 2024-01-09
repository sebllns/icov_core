/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup Encoder
 *  ICOV Encoder Lib
 *  @ingroup ICOV
 */

/** @file IcovMediaEncoder.h
 *  @brief Defines an actual ICOV IcovMediaEncoder.
 *  @ingroup IcovMediaEncoder
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.5.0
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

#include "encoder/FrameEncoder.h"
#include "encoder/OutputIcovBase.h"
#include "encoder/interface/IEncoder.h"
#include "omnidirectional_layer/Block.h"

#include <boost/filesystem/path.hpp>
#include <map>
#include <opencv2/opencv.hpp>

namespace icov {
namespace encoder {
class IcovMediaEncoder : public IMediaEncoder<cv::Mat> {
public:
  enum eOutputType { UNKNOWN = 0, BINARY = 1, TEXT = 2, COUNT };

  struct params {
    fs::path InputPath;
    fs::path OutputPath;
    eOutputType OutputType = eOutputType::UNKNOWN;
    FrameEncoder::params FrameEncoderParams;
    bool AllBlocks;
    omnidirectional_layer::Block::id_t StartBlock;
    size_t BlockCount;
    bool DummyViewport;

    params() = default;
  };

  IcovMediaEncoder(const params &p, const LdpcCoderParameters &LDPC_params);
  ~IcovMediaEncoder();

  void defaultEncode(float fov_x_degree, float viewportPadding,
                     float display_horizontal_ratio,
                     float viewport_image_format);

  const params &parameters() const;
  const LdpcCoderParameters &ldpc_parameters() const;
  const icov::encoder::OutputIcovBase &output_media() const;
  std::shared_ptr<icov::encoder::OutputIcovBase> output_ptr() const;

private:
  params m_params;
  std::unique_ptr<LdpcCoderParameters> m_LDPC_params;
  std::shared_ptr<icov::encoder::OutputIcovBase> m_output;

protected:
  // IMediaEncoder interface
  virtual void processInput(const icov::IInputMedia<cv::Mat> &input) override;

  // IMediaEncoder interface
  std::unique_ptr<icov::encoder::IFrameEncoder<cv::Mat>>
  createFrameEncoder() const override;

  std::shared_ptr<icov::encoder::OutputIcovBase> createOutputFile() const;
};
} // namespace encoder
} // namespace icov
