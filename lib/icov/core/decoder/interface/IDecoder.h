/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup IDecoder
 * IDecoder package (client side).
 *  @ingroup ICOV
 */

/** @file IDecoder.h
 *  @brief Defines an abstract ICOV IDecoder.
 *  @ingroup IDecoder
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

#include "Media.h"
#include "common.h"

#include "omnidirectional_layer/interface/IAccessBlockManager.h"
#include "omnidirectional_layer/interface/IPicture.h"

#include <memory>

namespace icov {
namespace decoder {

///////////////////////////////////////////////////////////////////////////////
///
/// @class IDecoder
/// @todo COMMENT !
///
///////////////////////////////////////////////////////////////////////////////
template <typename T> class IDecoder {
public:
  virtual ~IDecoder() = default;

  virtual void decode(std::istream &output, IInputMedia<T> &input,
                      const IPicture &picture,
                      const IAccessBlockManager &access_blocks,
                      const IBlockPredictionManager &block_prediction) = 0;

  virtual const IInputMedia<T> *media() const { return m_pMedia.get(); }

protected:
  virtual void processInput(const std::istream &input) = 0;
  std::unique_ptr<IInputMedia<T>> m_pMedia;
};

///////////////////////////////////////////////////////////////////////////////
///
/// @class IFrameDecoder
/// @todo COMMENT !
///
///////////////////////////////////////////////////////////////////////////////
template <typename T> class IFrameDecoder : public IDecoder<T> {
protected:
  int m_frame;

public:
  IFrameDecoder(int frame) : m_frame(frame) {}
  virtual void encode(std::ostream &output, IInputMedia<T> &input,
                      const IPicture &picture,
                      const IAccessBlockManager &access_blocks,
                      const IBlockPredictionManager &block_prediction);

protected:
  virtual void encodeBlock(std::ostream &output, const Block &b,
                           bool access_block,
                             const IBlockPredictionManager &block_prediction);
  virtual void encodeBlockWithPrediction(std::ostream &output, const Block &b,
                                         IBlockPredictionManager::prediction_t prediction) = 0;
  virtual std::vector<IBlockPredictionManager::prediction_t>
  getAvailablePredictions(const Block &b, bool access_block,
                          const IBlockPredictionManager &block_prediction) {
    return block_prediction.getAvailablePredictions(b, access_block);
  }

private:
  IFrameDecoder() = default;
  IFrameDecoder(const IFrameDecoder &) = default;
};

template <typename T>
void IFrameDecoder<T>::encode(std::ostream &output, IInputMedia<T> &input,
                              const IPicture &picture,
                              const IAccessBlockManager &access_blocks,
                              const IBlockPredictionManager &block_prediction) {

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
void IFrameDecoder<T>::encodeBlock(std::ostream &output, const Block &b,
                                   bool access_block,
                                   const IBlockPredictionManager &block_prediction) {
  for (IBlockPredictionManager::prediction_t p : getAvailablePredictions(b, access_block, block_prediction))
    encodeBlockWithPrediction(output, b, p);
}

///////////////////////////////////////////////////////////////////////////////
///
/// @class IMediaDecoder
/// @todo COMMENT !
///
///////////////////////////////////////////////////////////////////////////////
template <typename T> class IMediaDecoder : public IDecoder<T> {

public:
  virtual void encode(std::ostream &output, IInputMedia<T> &input,
                      const IPicture &picture,
                      const IAccessBlockManager &access_blocks,
                        const IBlockPredictionManager &block_prediction);

protected:
  virtual std::unique_ptr<IFrameDecoder<T>> createFrameDecoder(int frame) = 0;
};

template <typename T>
void IMediaDecoder<T>::encode(std::ostream &output, IInputMedia<T> &input,
                              const IPicture &picture,
                              const IAccessBlockManager &access_blocks,
                              const IBlockPredictionManager &block_prediction) {
  processInput(output, input);
  int frameCount = input.getFrameCount();

  for (int i = 0; i < frameCount; i++) {
    auto pFrameEnc = createFrameDecoder(i);
    pFrameEnc->encode(output, input, picture, access_blocks, block_prediction);
  }
}
} // namespace decoder
} // namespace icov
