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

#include "FastArray.h"

#include "icov_io.h"
#include "icov_stacktrace.h"
#include "ldpc_codec_layer/QaryDistribution.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include "video_layer/extracted_stream.h"
#include "video_layer/icov_stream.h"
#include <boost/serialization/singleton.hpp>
#include <cstdint>
#include <map>
#include <sstream>
#include <string>
#include <vector>
namespace icov {
namespace bitstream {

class VideoEncodingStreamCache
    : public boost::serialization::singleton<VideoEncodingStreamCache> {
  friend class boost::serialization::singleton<VideoEncodingStreamCache>;

public:
  struct ChannelBlockStreamDebugger {
    SymboleVec srcSymboles;
    Bitplanes srcBits;
    NumcodeIdxVec storedSyndromeSize;
    std::map<omnidirectional_layer::IBlockPredictionManager::prediction_t,
             SIStream>
        si_map;

    int bitplaneCount() const { return srcBits.size(); }
    int n() const { return srcSymboles.size(); }
  };

  typedef std::vector<ChannelBlockStreamDebugger>
      MultiChannelBlockStreamDebugger;
  typedef std::vector<MultiChannelBlockStreamDebugger> FrameStreamDebugger;
  typedef std::vector<FrameStreamDebugger> MultiFrameStreamDebugger;

  VideoEncodingStreamCache() = default;
  ~VideoEncodingStreamCache() = default;
  void setFrameCount(int count) { m_framesDbg.resize(count); }
  void clear() { m_framesDbg.clear(); }
  int getFrameCount() const { return m_framesDbg.size(); }
  const FrameStreamDebugger &getFrame(int i) const { return m_framesDbg[i]; }
  FrameStreamDebugger &getFrame(int i) { return m_framesDbg[i]; }

private:
  MultiFrameStreamDebugger m_framesDbg;
};

/**
 * @brief VideoStreamDebugger class provides debugging utilities for the video
 * stream. It is a singleton class. It is used to check bitstream and get the
 * debugging frames, blocks, channels, syndrome, etc.
 *
 * It provides functions to access debugging information for a particular
 * frame, block, channel and prediction type.
 * It also provides functions to check and assert the correctness
 * of bitplane count and bitplane syndromes with the ground truth.
 *
 */
class VideoStreamDebugger
    : public boost::serialization::singleton<VideoStreamDebugger> {
  friend class boost::serialization::singleton<VideoStreamDebugger>;

public:
  VideoStreamDebugger() { reset(); }

  /**
   * @brief Checks if the debugging frame ID is available i.e. is in the range
   * of the frame count.
   *
   * @return True if the debugging frame is available; False otherwise.
   */
  bool frameAvailable() const {
    return mDebuggingFrameID >= 0 &&
           mDebuggingFrameID <
               VideoEncodingStreamCache::get_const_instance().getFrameCount();
  }

  /**
   * @brief Checks if the debugging block ID is available i.e. in the range of
   * the blocks in the debugging frame.
   *
   * @return True if the debugging block is available; False otherwise.
   */
  bool blockAvailable() const {
    return (frameAvailable() && mDebuggingBlockID >= 0 &&
            mDebuggingBlockID < (int)getDebuggingFrame().size());
  }

  /**
   * @brief Checks if the debugging channel ID is available i.e. in the range of
   * the channels in the debugging block.
   *
   * @return True if the debugging channel is available; False otherwise.
   */
  bool channelAvailable() const {
    return (frameAvailable() && blockAvailable() && mDebuggingChannelID >= 0 &&
            mDebuggingChannelID < (int)getDebuggingBlock().size());
  }

  /**
   * @brief Checks if the debugging prediction ID is available i.e. in the SI
   * map of the debugging channel.
   *
   * @return True if the debugging prediction is available; False otherwise.
   */
  bool predictionAvailable() const {
    return (frameAvailable() && blockAvailable() && channelAvailable() &&
            getDebuggingChannel().si_map.count(mDebuggingPredictionID) == 1);
  }

  /**
   * @brief Gets the debugging frame based on the debugging frame ID.
   *
   * Clients can use this method to access the debugging frame and perform
   * further analysis on it.
   *
   * @return const reference to the debugging frame.
   */
  const VideoEncodingStreamCache::FrameStreamDebugger &
  getDebuggingFrame() const {
    icov::assert::check(frameAvailable(),
                        ICOV_INDICATOR_MSG(" ") + header().str() +
                            " Frame must be available: ID = " +
                            std::to_string(mDebuggingFrameID));
    return VideoEncodingStreamCache::get_const_instance().getFrame(
        mDebuggingFrameID);
  }

  /**
   * @brief Gets the debugging block based on the debugging block ID.
   *
   * @return const reference to the debugging block.
   */
  const VideoEncodingStreamCache::MultiChannelBlockStreamDebugger &
  getDebuggingBlock() const {
    icov::assert::check(blockAvailable(),
                        ICOV_INDICATOR_MSG(" ") + header().str() +
                            " Block must be available: ID = " +
                            std::to_string(mDebuggingBlockID));
    return getDebuggingFrame().at(mDebuggingBlockID);
  }

  /**
   * @brief Gets the debugging channel based on the debugging channel ID.
   *
   * @return const reference to the debugging channel.
   */
  const VideoEncodingStreamCache::ChannelBlockStreamDebugger &
  getDebuggingChannel() const {
    icov::assert::check(blockAvailable(),
                        ICOV_INDICATOR_MSG(" ") + header().str() +
                            " Channel must be available: ID = " +
                            std::to_string(mDebuggingChannelID));
    return getDebuggingBlock().at(mDebuggingChannelID);
  }

  /**
   * @brief Get the SIStream object for the current debugging prediction type.
   * @return const SIStream& The SIStream object.
   * @throw std::logic_error If prediction is not available.
   */
  const SIStream &getDebuggingSI() const {
    icov::assert::check(predictionAvailable(),
                        ICOV_INDICATOR_MSG(" ") + header().str() +
                            " Prediction must be available: ID = " +
                            omnidirectional_layer::BlockPrediction::toString(
                                mDebuggingPredictionID));
    return getDebuggingChannel().si_map.at(mDebuggingPredictionID);
  }

  /**
   * @brief Get the FrameStreamDebugger object for the current debugging frame.
   * @return VideoEncodingStreamCache::FrameStreamDebugger& The
   * FrameStreamDebugger object.
   * @throw std::logic_error If frame is not available.
   */
  VideoEncodingStreamCache::FrameStreamDebugger &getDebuggingFrame() {
    icov::assert::check(frameAvailable(),
                        ICOV_INDICATOR_MSG(" ") + header().str() +
                            " Frame must be available: ID = " +
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
    return getDebuggingChannel().si_map[mDebuggingPredictionID];
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

  /**
   * @brief Get the Syndrome object of the given bitplane.
   *
   * This function returns the Syndrome object associated with the given
   * bitplane index of the debugging channel.
   *
   * @param[in] bp The index of the bitplane to retrieve the Syndrome object.
   * @return const Syndrome& The Syndrome object of the given bitplane.
   * @throws std::out_of_range if the given bitplane index is out of range.
   * @throws std::logic_error if the prediction is not available.
   */
  const Syndrome &getBitplaneSyndrome(int bp) const {
    return getDebuggingSI().bitplanes.at(bp).syndrome;
  }

  /**
   * @brief Check if the Syndrome object of the given bitplane matches the
   provided Syndrome object.
   *
   * This function checks if the Syndrome object of the given bitplane matches
   * the provided Syndrome object.
   *
   * @param[in] bp The index of the bitplane to compare.
   * @param[in] s The Syndrome object to compare with.
   * @return bool True if the Syndrome objects match, false otherwise.
   * @throws std::out_of_range if the given bitplane index is out of range.
   * @throws std::logic_error if the prediction is not available.
   */
  bool checkSyndrome(int bp, const Syndrome &s) const {
    return getBitplaneSyndrome(bp) == s;
  }

  /**
   * @brief Assert that the Syndrome object of the given bitplane matches the
   * provided Syndrome object.
   *
   * This function asserts that the Syndrome object of the given bitplane
   * matches the provided Syndrome object. If the Syndrome objects do not match,
   * a logical exception is thrown with an error message containing the actual
   * and expected Syndrome objects.
   *
   * @param[in] bp The index of the bitplane to compare.
   * @param[in] s The Syndrome object to compare with.
   * @throws std::out_of_range if the given bitplane index is out of range.
   * @throws std::logic_error if the prediction is not available.
   * @throws std::logic_error if the Syndrome objects do not match.
   */
  void assertSyndromeEqual(int bp, const Syndrome &s) const {
    if (predictionAvailable() && !checkSyndrome(bp, s)) {
      std::stringstream sstr = header();
      sstr << "Bitplane = " << std::to_string(bp) << std::endl
           << "Syndrome does not match ground truth." << std::endl
           << "Syndrome " << s.to_string(" ") << std::endl
           << "GT " << getBitplaneSyndrome(bp).to_string(" ");
      const auto str = sstr.str();
      ICOV_THROW_LOGICAL_EXCEPTION(str);
    }
  }

  const BitVec &getBitplane(int bp) const {
    std::cout << __func__ << std::endl;
    return getDebuggingChannel().srcBits.at(bp);
  }

  bool checkBitplane(int bp, const BitVec &s) const {
    return getBitplane(bp) == s;
  }

  void assertBitplaneEqual(int bp, const BitVec &s) const {
    std::cout << __func__ << std::endl;

    if (predictionAvailable() && !checkBitplane(bp, s)) {
      std::stringstream sstr = header();
      sstr << "Bitplane = " << std::to_string(bp) << std::endl
           << "Bitplane does not match ground truth." << std::endl
           << "Value ";

      icov::io::write_vector(sstr, s, ' ');
      sstr << std::endl << "GT ";
      icov::io::write_vector(sstr, getBitplane(bp), ' ');
      sstr << std::endl;
      const auto str = sstr.str();
      ICOV_THROW_LOGICAL_EXCEPTION(str);
    }

    std::cout << "END " << __func__ << std::endl;
  }

  const SymboleVec &getSymboles() const {
    std::cout << __func__ << std::endl;
    return getDebuggingChannel().srcSymboles;
  }

  bool checkSymboles(const SymboleVec &s) const { return getSymboles() == s; }

  void assertSymbolesEqual(const SymboleVec &s) const {
    std::cout << __func__ << std::endl;

    if (predictionAvailable() && !checkSymboles(s)) {
      std::stringstream sstr = header();
      sstr << "Symboles does not match ground truth." << std::endl << "Value ";

      icov::io::write_vector(sstr, s, ' ');
      sstr << std::endl << "GT ";
      icov::io::write_vector(sstr, getSymboles(), ' ');
      sstr << std::endl;
      const auto str = sstr.str();
      ICOV_THROW_LOGICAL_EXCEPTION(str);
    }

    std::cout << "END " << __func__ << std::endl;
  }

  NumcodeIdx getStoredNumcodeIdx(int bp) const {
    return getDebuggingChannel().storedSyndromeSize.at(bp);
  }

  bool checkStoredNumcodeIdx(int bp, NumcodeIdx val) const {
    return getStoredNumcodeIdx(bp) == val;
  }

  void assertStoredNumcodeIdxEqual(int bp, NumcodeIdx val) const {
    if (channelAvailable() && !checkStoredNumcodeIdx(bp, val)) {
      std::stringstream sstr = header();
      sstr << "Stored syndrom size does not match ground truth." << std::endl
           << "Bitplane = " << bp << std::endl
           << "Value = " << val << std::endl
           << "GT = " << getStoredNumcodeIdx(bp);
      const auto str = sstr.str();
      ICOV_THROW_LOGICAL_EXCEPTION(str);
    }
  }

  NumcodeIdx getNumcodeIdx(int bp) const {
    return getDebuggingSI().bitplanes.at(bp).numcode;
  }

  /**
   * @brief Get the number of bitplanes in the current debugging channel.
   *
   * This function returns the number of bitplanes in the current debugging
   * channel. It is used to verify if the predicted bitplane count matches the
   * actual bitplane count.
   *
   * @throws std::logic_error if the prediction is not available.
   * @return int The number of bitplanes in the current debugging channel.
   */
  int getBitplaneCount() const { return getDebuggingChannel().bitplaneCount(); }

  /**
   * @brief Checks if the bitplane count matches the predicted count.
   *
   * This function checks if the number of bitplanes in the current debugging
   * channel matches the given predicted bitplane count.
   *
   * @param bp_count The predicted bitplane count.
   * @return true if the bitplane count matches the predicted count, false
   * otherwise.
   */
  bool checkBitplaneCount(int bp_count) const {
    return getBitplaneCount() == bp_count;
  }

  /**
   * @brief Asserts that the bitplane count matches the predicted count.
   *
   * This function asserts that the number of bitplanes in the current
   * debugging channel matches the given predicted bitplane count. If the
   * prediction is available and the counts do not match, a logical exception is
   * thrown with a detailed error message.
   *
   * @param bp_count The predicted bitplane count.
   * @throws std::logic_error if the prediction is available and the bitplane
   * counts do not match.
   */
  void assertBitplaneCountEqual(int bp_count) const {
    if (channelAvailable() && !checkBitplaneCount(bp_count)) {
      std::stringstream sstr = header();
      sstr << "Bitplane count does not match ground truth." << std::endl
           << "Value = " << bp_count << std::endl
           << "GT = " << getBitplaneCount();
      const auto str = sstr.str();
      ICOV_THROW_LOGICAL_EXCEPTION(str);
    }
  }

  int getDirMode() const {
    return getPredDirModeByteValue(true, getDebuggingSI().mode);
  }

  bool checkDirMode(int value) const { return getDirMode() == value; }

  void assertDirModeEqual(int value) const {
    if (predictionAvailable() && !checkDirMode(value)) {
      std::stringstream sstr = header();
      sstr << "Dirmode does not match ground truth." << std::endl
           << "Value = " << value << std::endl
           << "GT = " << getDirMode();
      const auto str = sstr.str();
      ICOV_THROW_LOGICAL_EXCEPTION(str);
    }
  }

  NumcodeIdx getNumcode(int bp) const {
    return getDebuggingSI().bitplanes.at(bp).numcode;
  }

  bool checkNumcode(int bp, NumcodeIdx value) const {
    return getNumcode(bp) == value;
  }

  void assertNumcodeEqual(int bp, NumcodeIdx value) const {
    if (predictionAvailable() && !checkNumcode(bp, value)) {
      std::stringstream sstr = header();
      sstr << "Bitplane = " << bp << std::endl
           << "Numcode does not match ground truth." << std::endl
           << "Value = " << value << std::endl
           << "GT = " << getNumcode(bp);
      const auto str = sstr.str();
      ICOV_THROW_LOGICAL_EXCEPTION(str);
    }
  }

  float
  getQaryParam(ldpc_codec_layer::QaryDistribution::QaryParamsIndex idx) const {
    return getDebuggingSI().pz.getRawParam(idx);
  }

  bool checkQaryParam(ldpc_codec_layer::QaryDistribution::QaryParamsIndex idx,
                      float value) const {
    return getQaryParam(idx) == value;
  }

  void
  assertQaryParamEqual(ldpc_codec_layer::QaryDistribution::QaryParamsIndex idx,
                       float value) const {
    if (predictionAvailable() && !checkQaryParam(idx, value)) {
      std::stringstream sstr = header();
      sstr << "QaryParam does not match ground truth." << std::endl
           << "Index = " << idx << std::endl
           << "Value = " << value << std::endl
           << "GT = " << getQaryParam(idx);
      const auto str = sstr.str();
      ICOV_THROW_LOGICAL_EXCEPTION(str);
    }
  }

  bool codingSIFlag() const { return !getDebuggingSI().bitplanes.empty(); }

  bool checkcodingSIFlag(bool value) const { return codingSIFlag() == value; }

  void assertcodingSIEqual(bool value) const {
    if (predictionAvailable() && !checkcodingSIFlag(value)) {
      std::stringstream sstr = header();
      sstr << "Coding SI flag does not match ground truth." << std::endl
           << "Value = " << value << std::endl
           << "GT = " << codingSIFlag();
      const auto str = sstr.str();
      ICOV_THROW_LOGICAL_EXCEPTION(str);
    }
  }

  /**
   * @brief Creates a header string for debugging messages.
   *
   * This function creates a header string for debugging messages, which
   * includes the current debugging frame ID, block ID, channel ID, and
   * prediction ID.
   *
   *   @return std::stringstream A stringstream containing the header string.
   */
  std::stringstream header() const {
    std::stringstream sstr;
    sstr << std::endl
         << "Frame = " << mDebuggingFrameID << std::endl
         << "Block = " << mDebuggingBlockID << std::endl
         << "Channel = " << mDebuggingChannelID << std::endl
         << "Prediction = "
         << omnidirectional_layer::BlockPrediction::toString(
                mDebuggingPredictionID)
         << std::endl;
    return sstr;
  }

  /**
   * @brief Resets the debugging information.
   * This function resets the debugging information by setting the debugging
   * frame ID, block ID, channel ID, and prediction ID to their default values.
   */
  void reset() {
    setDebuggingBlockID(-1);
    setDebuggingChannelID(-1);
    setDebuggingFrameID(0 /*-1*/);
    setDebuggingPredictionID(
        omnidirectional_layer::BlockPrediction::ePredType::UNKNOWN);
  }

private:
  int mDebuggingFrameID;   ///< The debugging frame ID.
  int mDebuggingBlockID;   ///< The debugging block ID.
  int mDebuggingChannelID; ///< The debugging channel ID.
  omnidirectional_layer::BlockPrediction::ePredType
      mDebuggingPredictionID; ///< The debugging prediction ID.
};

} // namespace bitstream
} // namespace icov
