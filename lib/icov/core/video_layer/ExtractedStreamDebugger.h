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
#include <vector>
namespace icov {
namespace bitstream {

class VideoEncodingStreamCache
    : public boost::serialization::singleton<VideoEncodingStreamCache> {
  friend class boost::serialization::singleton<VideoEncodingStreamCache>;

public:
  typedef std::map<omnidirectional_layer::IBlockPredictionManager::prediction_t, SIStream>
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

    omnidirectional_layer::BlockPrediction::predType DebuggingPrediction;

  bool frameAvailable() const {
    return mDebuggingFrame <
           VideoEncodingStreamCache::get_const_instance().getFrameCount();
  }

  //  const VideoEncodingStreamCache::FrameStreamDebugger &
  //  getDebuggingFrame() const {
  //    icov::helpers::assert::check(frameAvailable(), "No more frame to
  //    debug"); return VideoEncodingStreamCache::get_const_instance().getFrame(
  //        DebuggingFrame);
  //  }
  const SIStream &getDebuggingSI() const {
    icov::helpers::assert::check(frameAvailable(), "No more frame to debug");
    return VideoEncodingStreamCache::get_const_instance()
        .getFrame(mDebuggingFrame)
        .at(mDebuggingBlock)
        .at(mDebuggingChannel)
        .at(DebuggingPrediction);
  }

  SIStream &getDebuggingSI() {
    icov::helpers::assert::check(frameAvailable(), "No more frame to debug");
    return VideoEncodingStreamCache::get_mutable_instance().getFrame(
        mDebuggingFrame)[mDebuggingBlock][mDebuggingChannel]
                        [DebuggingPrediction];
  }

  int debuggingFrame() const { return mDebuggingFrame; }
  void setDebuggingFrame(int newDebuggingFrame) {
    mDebuggingFrame = newDebuggingFrame;
  }
  int debuggingBlock() const { return mDebuggingBlock; }
  void setDebuggingBlock(int newDebuggingBlock) {
    mDebuggingBlock = newDebuggingBlock;
  }
  int debuggingChannel() const { return mDebuggingChannel; }
  void setDebuggingChannel(int newDebuggingChannel) {
    mDebuggingChannel = newDebuggingChannel;
  }

private:
  int mDebuggingFrame;
  int mDebuggingBlock;
  int mDebuggingChannel;
};
} // namespace bitstream
} // namespace icov
