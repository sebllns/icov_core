/****************************************************************************/
/** @addtogroup icov-core
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup IEncoder
 * IEncoder package (client side).
 *  @ingroup icov-core
 */

/** @file IEncoder.h
 *  @brief Defines an abstract ICOV IEncoder.
 *  @ingroup IEncoder
 *  @author Sébastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
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

#include "Media.h"
#include "common.h"
#include "omnidirectional_layer/BlockPrediction.h"
#include "omnidirectional_layer/interface/IAccessBlockManager.h"
#include "omnidirectional_layer/interface/IPicture.h"

namespace icov {
namespace encoder {

///////////////////////////////////////////////////////////////////////////////
///
/// @class IEncoder
/// @todo COMMENT !
///
///////////////////////////////////////////////////////////////////////////////
template <typename T> class IEncoder {
public:
  virtual ~IEncoder() = default;
  virtual void encode(std::ostream &output, IInputMedia<T> &input,
                      const IPicture &picture,
                      const IAccessBlockManager &access_blocks,
                      const IBlockPrediction &block_prediction) = 0;

protected:
  virtual void processInput(std::ostream &output,
                            const IInputMedia<T> &input) = 0;
};

///////////////////////////////////////////////////////////////////////////////
///
/// @class IFrameEncoder
/// @todo COMMENT !
///
///////////////////////////////////////////////////////////////////////////////
template <typename T> class IFrameEncoder : public IEncoder<T> {
protected:
  int m_frame;

public:
  IFrameEncoder(int frame) : m_frame(frame) {}
  virtual void encode(std::ostream &output, IInputMedia<T> &input,
                      const IPicture &picture,
                      const IAccessBlockManager &access_blocks,
                      const IBlockPrediction &block_prediction);

protected:
  virtual void encodeBlock(std::ostream &output, const Block &b,
                           bool access_block,
                           const IBlockPrediction &block_prediction);
  virtual void encodeBlockWithPrediction(std::ostream &output, const Block &b,
                                         uint16_t prediction) = 0;
  virtual std::vector<uint16_t>
  getAvailablePredictions(const Block &b, bool access_block,
                          const IBlockPrediction &block_prediction) {
    return block_prediction.getAvailablePredictions(b, access_block);
  }

private:
  IFrameEncoder() = default;
  IFrameEncoder(const IFrameEncoder &) = default;
};

template <typename T>
void IFrameEncoder<T>::encode(std::ostream &output, IInputMedia<T> &input,
                              const IPicture &picture,
                              const IAccessBlockManager &access_blocks,
                              const IBlockPrediction &block_prediction) {

  input.setFrame(m_frame);

  ICOV_ASSERT_MSG(input.getPixelCount() > 0, "Cannot encode empty frame");
  ICOV_ASSERT_MSG(input.getHeight() == picture.height() &&
                      input.getWidth() == picture.width(),
                  "Picture is not the right size");

  this->processInput(output, input);
  for (const auto &b : picture.icus()) {
    encodeBlock(output, b, access_blocks.isAccessBlock(b.getId(Block::SELF)),
                block_prediction);
  }
}

template <typename T>
void IFrameEncoder<T>::encodeBlock(std::ostream &output, const Block &b,
                                   bool access_block,
                                   const IBlockPrediction &block_prediction) {
  for (uint16_t p : getAvailablePredictions(b, access_block, block_prediction))
    encodeBlockWithPrediction(output, b, p);
}

///////////////////////////////////////////////////////////////////////////////
///
/// @class IMediaEncoder
/// @todo COMMENT !
///
///////////////////////////////////////////////////////////////////////////////
template <typename T> class IMediaEncoder : public IEncoder<T> {

public:
  virtual void encode(std::ostream &output, IInputMedia<T> &input,
                      const IPicture &picture,
                      const IAccessBlockManager &access_blocks,
                      const IBlockPrediction &block_prediction);

protected:
  virtual std::unique_ptr<IFrameEncoder<T>> createFrameEncoder(int frame) = 0;
};

template <typename T>
void IMediaEncoder<T>::encode(std::ostream &output, IInputMedia<T> &input,
                              const IPicture &picture,
                              const IAccessBlockManager &access_blocks,
                              const IBlockPrediction &block_prediction) {
  processInput(output, input);
  int frameCount = input.getFrameCount();

  for (int i = 0; i < frameCount; i++) {
    auto pFrameEnc = createFrameEncoder(i);
    pFrameEnc->encode(output, input, picture, access_blocks, block_prediction);
  }
}
} // namespace encoder
} // namespace icov
