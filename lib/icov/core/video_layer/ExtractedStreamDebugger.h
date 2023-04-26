/****************************************************************************/
/** @addtogroup Omnidirectional
 *  Omnidirectional is the adaptation of the coder into 360 context.
 *  @ingroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @file decoder.h
 *  @brief Defines an actual ICOV decoder.
 *  @ingroup Omnidirectional
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

#include "helpers.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include "video_layer/extracted_stream.h"
#include <boost/serialization/singleton.hpp>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
namespace icov {
namespace bitstream {

class VideoEncodingStreamCache
    : public boost::serialization::singleton<VideoEncodingStreamCache> {
  friend class boost::serialization::singleton<VideoEncodingStreamCache>;

public:
  typedef std::map<omnidirectional_layer::IBlockPredictionManager::prediction_t,
                   SIStream>
      ChannelBlockStreamDebugger;
  typedef std::vector<ChannelBlockStreamDebugger>
      MultiChannelBlockStreamDebugger;
  typedef std::vector<MultiChannelBlockStreamDebugger> FrameStreamDebugger;
  typedef std::vector<FrameStreamDebugger> MultiFrameStreamDebugger;

  VideoEncodingStreamCache() = default;
  ~VideoEncodingStreamCache() = default;
  void setFrameCount(int count) { m_framesDbg.resize(count); }
  int getFrameCount() const { return m_framesDbg.size(); }
  const FrameStreamDebugger &getFrame(int i) const { return m_framesDbg[i]; }
  FrameStreamDebugger &getFrame(int i) { return m_framesDbg[i]; }

private:
  MultiFrameStreamDebugger m_framesDbg;
};

class VideoStreamDebugger
    : public boost::serialization::singleton<VideoStreamDebugger> {
  friend class boost::serialization::singleton<VideoStreamDebugger>;

public:
  VideoStreamDebugger() = default;

  bool frameAvailable() const {
    return mDebuggingFrameID <
           VideoEncodingStreamCache::get_const_instance().getFrameCount();
  }

  bool blockAvailable() const {
    return (frameAvailable() &&
            mDebuggingBlockID < (int)getDebuggingFrame().size());
  }

  bool channelAvailable() const {
    return (frameAvailable() && blockAvailable() &&
            mDebuggingChannelID < (int)getDebuggingBlock().size());
  }

  bool predictionAvailable() const {
    return (frameAvailable() && blockAvailable() && channelAvailable() &&
            getDebuggingChannel().count(mDebuggingPredictionID) == 1);
  }

  const VideoEncodingStreamCache::FrameStreamDebugger &
  getDebuggingFrame() const {
    icov::helpers::assert::check(frameAvailable(),
                                 "Frame is not available ID = " +
                                     std::to_string(mDebuggingFrameID));
    return VideoEncodingStreamCache::get_const_instance().getFrame(
        mDebuggingFrameID);
  }

  const VideoEncodingStreamCache::MultiChannelBlockStreamDebugger &
  getDebuggingBlock() const {
    icov::helpers::assert::check(blockAvailable(),
                                 "Block is not available ID = " +
                                     std::to_string(mDebuggingBlockID));
    return getDebuggingFrame().at(mDebuggingBlockID);
  }

  const VideoEncodingStreamCache::ChannelBlockStreamDebugger &
  getDebuggingChannel() const {
    icov::helpers::assert::check(blockAvailable(),
                                 "Channel is not available ID = " +
                                     std::to_string(mDebuggingChannelID));
    return getDebuggingBlock().at(mDebuggingChannelID);
  }

  const SIStream &getDebuggingSI() const {
    icov::helpers::assert::check(
        predictionAvailable(),
        "Prediction is not available ID = " +
            omnidirectional_layer::BlockPrediction::toString(
                mDebuggingPredictionID));
    return getDebuggingChannel().at(mDebuggingPredictionID);
  }

  VideoEncodingStreamCache::FrameStreamDebugger &getDebuggingFrame() {
    icov::helpers::assert::check(frameAvailable(),
                                 "Frame is not available ID = " +
                                     std::to_string(mDebuggingFrameID));
    return VideoEncodingStreamCache::get_mutable_instance().getFrame(
        mDebuggingFrameID);
  }

  VideoEncodingStreamCache::MultiChannelBlockStreamDebugger &
  getDebuggingBlock() {
    if (!blockAvailable()) {
      getDebuggingFrame().resize(mDebuggingBlockID + 1);
    }
    return getDebuggingFrame().at(mDebuggingBlockID);
  }

  VideoEncodingStreamCache::ChannelBlockStreamDebugger &getDebuggingChannel() {
    if (!channelAvailable()) {
      getDebuggingBlock().resize(mDebuggingChannelID + 1);
    }
    return getDebuggingBlock().at(mDebuggingChannelID);
  }

  SIStream &getDebuggingSI() {
    return getDebuggingChannel()[mDebuggingPredictionID];
  }

  int debuggingFrameID() const { return mDebuggingFrameID; }
  void setDebuggingFrameID(int newDebuggingFrame) {
    mDebuggingFrameID = newDebuggingFrame;
  }
  int debuggingBlockID() const { return mDebuggingBlockID; }
  void setDebuggingBlockID(int newDebuggingBlock) {
    mDebuggingBlockID = newDebuggingBlock;
  }
  int debuggingChannelID() const { return mDebuggingChannelID; }
  void setDebuggingChannelID(int newDebuggingChannel) {
    mDebuggingChannelID = newDebuggingChannel;
  }

  omnidirectional_layer::BlockPrediction::ePredType
  debuggingPredictionID() const {
    return mDebuggingPredictionID;
  }
  void
  setDebuggingPredictionID(omnidirectional_layer::BlockPrediction::ePredType
                               newDebuggingPrediction) {
    mDebuggingPredictionID = newDebuggingPrediction;
  }

private:
  int mDebuggingFrameID;
  int mDebuggingBlockID;
  int mDebuggingChannelID;
  omnidirectional_layer::BlockPrediction::ePredType mDebuggingPredictionID;
};

} // namespace bitstream
} // namespace icov
