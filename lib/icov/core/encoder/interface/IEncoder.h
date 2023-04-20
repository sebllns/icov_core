/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup IEncoder
 * IEncoder package (client side).
 *  @ingroup ICOV
 */

/** @file IEncoder.h
 *  @brief Defines an abstract ICOV IEncoder.
 *  @ingroup IEncoder
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

#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include "omnidirectional_layer/interface/IPicture.h"
#include "video_layer/ExtractedStreamDebugger.h"

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
  virtual void encode(IInputMedia<T> &input,
                      const omnidirectional_layer::IPicture &picture,
                      const omnidirectional_layer::IBlockPredictionManager
                          &block_prediction) = 0;

protected:
  virtual void processInput(const IInputMedia<T> &input) = 0;
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
  IFrameEncoder() = default;
  virtual void encode(
      IInputMedia<T> &input, const omnidirectional_layer::IPicture &picture,
      const omnidirectional_layer::IBlockPredictionManager &block_prediction);

  int frame() const { return m_frame; }
  void setFrame(int newFrame) { m_frame = newFrame; }

protected:
  virtual void encodeBlock(
      const omnidirectional_layer::Block &b, bool access_block,
      const omnidirectional_layer::IBlockPredictionManager &block_prediction);
  virtual void encodeBlockWithPrediction(
      const omnidirectional_layer::Block &b,
      omnidirectional_layer::IBlockPredictionManager::prediction_t
          prediction) = 0;
  virtual std::vector<
      omnidirectional_layer::IBlockPredictionManager::prediction_t>
  getAvailablePredictions(
      const omnidirectional_layer::Block &b, bool access_block,
      const omnidirectional_layer::IBlockPredictionManager &block_prediction) {
    return block_prediction.getAvailablePredictions(b, access_block);
  }

private:
  IFrameEncoder(const IFrameEncoder &) = default;
};

template <typename T>
void IFrameEncoder<T>::encode(
    IInputMedia<T> &input, const omnidirectional_layer::IPicture &picture,
    const omnidirectional_layer::IBlockPredictionManager &block_prediction) {

#ifdef ICOV_ASSERTION_ENABLED
  ICOV_ASSERT_MSG(input.getPixelCount() > 0, "Cannot encode empty frame");
  ICOV_ASSERT_MSG(input.getHeight() == picture.height() &&
                      input.getWidth() == picture.width(),
                  "Picture is not the right size");
#endif

  this->processInput(input);
  for (const auto &b : picture.icus()) {
    bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingBlock(
        b.getIdSelf() - 1);
    encodeBlock(b, b.isAccessBlock(), block_prediction);
  }
}

template <typename T>
void IFrameEncoder<T>::encodeBlock(
    const omnidirectional_layer::Block &b, bool access_block,
    const omnidirectional_layer::IBlockPredictionManager
        &block_prediction_mgr) {
  for (omnidirectional_layer::IBlockPredictionManager::prediction_t p :
       getAvailablePredictions(b, access_block, block_prediction_mgr)) {
    // bitstream::VideoStreamDebugger::get_mutable_instance()
    //   .DebuggingPrediction.
    encodeBlockWithPrediction(b, p);
  }
}

///////////////////////////////////////////////////////////////////////////////
///
/// @class IMediaEncoder
/// @todo COMMENT !
///
///////////////////////////////////////////////////////////////////////////////
template <typename T> class IMediaEncoder : public IEncoder<T> {

public:
  virtual void encode(
      IInputMedia<T> &input, const omnidirectional_layer::IPicture &picture,
      const omnidirectional_layer::IBlockPredictionManager &block_prediction);

protected:
  virtual std::unique_ptr<IFrameEncoder<T>> createFrameEncoder() const = 0;
};

template <typename T>
void IMediaEncoder<T>::encode(
    IInputMedia<T> &input, const omnidirectional_layer::IPicture &picture,
    const omnidirectional_layer::IBlockPredictionManager &block_prediction) {
  this->processInput(input);
  int frameCount = input.getFrameCount();
  auto pFrameEnc = createFrameEncoder();

  for (int i = 0; i < frameCount; i++) {
    ICOV_TRACE << "Encode frame " << i;
    bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingFrame(i);
    input.setFrame(i);
    pFrameEnc->setFrame(i);
    pFrameEnc->encode(input, picture, block_prediction);
  }
}
} // namespace encoder
} // namespace icov
