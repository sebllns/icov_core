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

#include "Media.h"

#include "icov_log.h"
#include "icov_request.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include "omnidirectional_layer/interface/IPicture.h"
#include "video_layer/ExtractedStreamDebugger.h"
#include <algorithm>
#include <exception>
#include <vector>

namespace icov {
namespace encoder {

///////////////////////////////////////////////////////////////////////////////
///
/// @class IEncoder
/// @brief Interface for encoding media data.
///
///////////////////////////////////////////////////////////////////////////////
template <typename T> class IEncoder {
public:
  virtual ~IEncoder() = default;

  /**
   * @brief Encodes the input 360 media data.
   *
   * @param input The input 360 media data to encode.
   * @param picture The 360 picture to encode the media data into.
   * @param block_prediction The block prediction manager for encoding blocks.
   */
  virtual void encode(IInputMedia<T> &input,
                      const omnidirectional_layer::IPicture &picture,
                      const omnidirectional_layer::IBlockPredictionManager
                          &block_prediction) = 0;

protected:
  /**
   * @brief Processes the input media data.
   *
   * @param input The input media data to process.
   */
  virtual void processInput(const IInputMedia<T> &input) = 0;
};

class IBitplaneEncoder {
public:
  struct StoredRate {
    NumcodeIdx c; // rate quantized
    Rate r;       // actual rate value
    uint m;       // code length

    StoredRate(const ldpc_codec_layer::IInteractiveCoder &enc, NumcodeIdx num) {

    }
  };

  virtual ~IBitplaneEncoder() = default;

  icov::Syndrome stored_synd;

  StoredRate stored_synd_rate;
  icov::Rate noSourceThreshold;
  bool sourceFlag;

  // one numcode per SI and per bitplane
  std::map<omnidirectional_layer::IBlockPredictionManager::prediction_t,
           StoredRate>
      stored_numcodes;
};

class IPredictionEncoder {
public:
  virtual ~IPredictionEncoder() = default;

  /**
   * @brief Encodes a block of data with a specific prediction.
   *
   * @param b The block to encode.
   * @param prediction The prediction type for encoding the block.
   */
  virtual void encodeBlockWithPrediction(
      const omnidirectional_layer::Block &b,
      omnidirectional_layer::IBlockPredictionManager::prediction_t
          prediction) = 0;
};

class IChannelEncoder {
public:
  IChannelEncoder(unsigned char comp_id,
                  const omnidirectional_layer::Block &parent,
                  const omnidirectional_layer::IBlockPredictionManager
                      &block_prediction_mgr)
      : m_comp_id(comp_id), m_block(parent),
        m_available_pred(
            block_prediction_mgr.getAvailablePredictions(m_block)) {}

  virtual ~IChannelEncoder() = default;

  /**
   * @brief Encodes a block of data.
   *
   * @param b The block to encode.
   * @param block_prediction The block prediction manager for encoding
   * blocks.
   */
  virtual void encodeBlock() {
#pragma omp parallel for
    for (omnidirectional_layer::IBlockPredictionManager::prediction_t p :
         m_available_pred) {
      // encodeBlockWithPrediction(m_block, p);
    }
  }

private:
  const unsigned char m_comp_id;
  const omnidirectional_layer::Block &m_block;
  const std::vector<
      omnidirectional_layer::IBlockPredictionManager::prediction_t>
      m_available_pred;
};

class IBlockEncoder {
public:
  virtual ~IBlockEncoder() = default;

  IBlockEncoder(unsigned char numcomp) : m_numcomp(numcomp) {}

  /**
   * @brief Encodes a block of data.
   *
   * @param b The block to encode.
   * @param block_prediction The block prediction manager for encoding blocks.
   */
  virtual void encodeBlock(const omnidirectional_layer::Block &b,
                           const omnidirectional_layer::IBlockPredictionManager
                               &block_prediction_mgr) {

    for (unsigned char c = 0; c < m_numcomp; ++c) {
      auto ch_enc = createChannelEncoder(c, b, block_prediction_mgr);
      ch_enc->encodeBlock();
    }
  }

protected:
  /**
   * @brief Creates a Channel encoder for encoding individual block.
   *
   * @return A unique pointer to the Channel encoder.
   */
  virtual std::unique_ptr<IChannelEncoder>
  createChannelEncoder(unsigned char comp_id,
                       const omnidirectional_layer::Block &parent,
                       const omnidirectional_layer::IBlockPredictionManager
                           &block_prediction_mgr) const = 0;

  unsigned char m_numcomp;
};

///////////////////////////////////////////////////////////////////////////////
///
/// @class IFrameEncoder
/// @brief Interface for encoding individual frames of media data.
///
///////////////////////////////////////////////////////////////////////////////
// template <typename T> class IFrameEncoder : public IEncoder<T> {
// protected:
//  int m_frame; ///< The current frame being encoded.

// public:
//  virtual ~IFrameEncoder() = default;
//  IFrameEncoder() = default;

//  /**
//   * @brief Encodes the input 360 media data for a specific frame.
//   *
//   * @param input The input 360 media data to encode.
//   * @param picture The 360 picture to encode the media data into.
//   * @param block_prediction The block prediction manager for encoding blocks.
//   */
//  virtual void encode(
//      IInputMedia<T> &input, const omnidirectional_layer::IPicture &picture,
//      const omnidirectional_layer::IBlockPredictionManager &block_prediction);

//  /**
//   * @brief Returns the current frame being encoded.
//   *
//   * @return The current frame being encoded.
//   */
//  int frame() const { return m_frame; }

//  /**
//   * @brief Sets the current frame to be encoded.
//   *
//   * @param newFrame The frame number to set.
//   */
//  void setFrame(int newFrame) { m_frame = newFrame; }

// protected:
//  /**
//   * @brief Creates a block encoder for encoding individual block.
//   *
//   * @return A unique pointer to the block encoder.
//   */
//  virtual std::unique_ptr<IFrameEncoder<T>> createBlockEncoder() const = 0;

// private:
//  IFrameEncoder(const IFrameEncoder &) = default;
//};

// template <typename T>
// void IFrameEncoder<T>::encode(
//    IInputMedia<T> &input, const omnidirectional_layer::IPicture &picture,
//    const omnidirectional_layer::IBlockPredictionManager &block_prediction) {

//#ifdef ICOV_ASSERTION_ENABLED
//  ICOV_ASSERT_MSG(input.getPixelCount() > 0, "Cannot encode empty frame");
//  ICOV_ASSERT_MSG(input.getHeight() == picture.height() &&
//                      input.getWidth() == picture.width(),
//                  "Picture is not the right size");
//#endif

//  this->processInput(input);
//  auto pBlockEnc = createBlockEncoder();
//  for (const auto &b : picture.icus()) {
//    bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingBlockID(
//        b.getIdSelf() - 1);
//    pBlockEnc->encodeBlock(b, block_prediction);
//  }
//}

///////////////////////////////////////////////////////////////////////////////
///
/// @class IFrameEncoder
/// @todo UPDATE THE CLASS
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
    return block_prediction.getAvailablePredictions(b);
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
    try {
      if (!b.hidden()) {
        bitstream::VideoStreamDebugger::get_mutable_instance()
            .setDebuggingBlockID(b.getIdSelf() - 1);
        encodeBlock(b, b.isAccessBlock(), block_prediction);
      }
    } catch (const std::exception &e) {
      ICOV_ERROR << "Catched error while encoding block " << b.getIdSelf();
      ICOV_ERROR << e.what();
      ICOV_ERROR << boost::current_exception_diagnostic_information();
    } catch (...) {
      ICOV_ERROR << "Unknown error while encoding block " << b.getIdSelf();
      ICOV_ERROR << boost::current_exception_diagnostic_information();
    }
  }
}

template <typename T>
void IFrameEncoder<T>::encodeBlock(
    const omnidirectional_layer::Block &b, bool access_block,
    const omnidirectional_layer::IBlockPredictionManager
        &block_prediction_mgr) {
  for (omnidirectional_layer::IBlockPredictionManager::prediction_t p :
       getAvailablePredictions(b, access_block, block_prediction_mgr)) {
    encodeBlockWithPrediction(b, p);
  }
}

///////////////////////////////////////////////////////////////////////////////
///
/// @class IMediaEncoder
/// @brief Interface for encoding media data consisting of multiple frames.
///
///////////////////////////////////////////////////////////////////////////////
template <typename T> class IMediaEncoder : public IEncoder<T> {

public:
  IMediaEncoder() = default;

  /**
   * @brief Encodes the input 360 media data consisting of multiple frames.
   *
   * @param input The input 360 media data to encode.
   * @param picture The 360 picture to encode the media data into.
   * @param block_prediction The block prediction manager for encoding blocks.
   */
  virtual void encode(
      IInputMedia<T> &input, const omnidirectional_layer::IPicture &picture,
      const omnidirectional_layer::IBlockPredictionManager &block_prediction);

  frame_t maxFrameCount() const { return m_max_frame_count; }

  void setMaxFrameCount(frame_t newMax_frame_count) {
    m_max_frame_count = newMax_frame_count;
  }

protected:
  /**
   * @brief Creates a frame encoder for encoding individual frames.
   *
   * @return A unique pointer to the frame encoder.
   */
  virtual std::unique_ptr<IFrameEncoder<T>> createFrameEncoder() const = 0;

private:
  frame_t m_max_frame_count;
  frame_t m_start_frame;
};

template <typename T>
void IMediaEncoder<T>::encode(
    IInputMedia<T> &input, const omnidirectional_layer::IPicture &picture,
    const omnidirectional_layer::IBlockPredictionManager &block_prediction) {
  this->processInput(input);
  auto pFrameEnc = createFrameEncoder();
  frame_t i = 0;

  while (i < input.getFrameCount() &&
         (m_max_frame_count == 0 || i < m_max_frame_count)) {
    frame_t f = i + m_start_frame;
    try {
      ICOV_TRACE << "Encode frame " << f;
      bitstream::VideoStreamDebugger::get_mutable_instance()
          .setDebuggingFrameID(i);
      input.setFrame(f);
      pFrameEnc->setFrame(i);
      pFrameEnc->encode(input, picture, block_prediction);
      ++i;
    } catch (const std::exception &e) {
      ICOV_ERROR << "Catched error while encoding frame " << f;
      ICOV_ERROR << e.what();
      ICOV_ERROR << boost::current_exception_diagnostic_information();
    } catch (...) {
      ICOV_ERROR << "Unknown error while encoding frame " << f;
      ICOV_ERROR << boost::current_exception_diagnostic_information();
    }
  }
}

} // namespace encoder
} // namespace icov
