/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup ICOV
 */

/** @file ContentParameterSet.h
 *  @brief ContentParameterSet VCL data for ICOV coder
 *  @ingroup common
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

#include "icov_assert.h"
#include "icov_exception.h"
#include "icov_types.h"
#include "ldpc_codec_layer/QaryDistribution.h"
#include "ldpc_codec_layer/interface/IInteractiveCoder.h"
#include "video_layer/icov_stream.h"

#include <array>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/queue.hpp>
#include <boost/serialization/vector.hpp>

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <queue>
#include <sys/types.h>
#include <vector>

#define BITSTREAM_SIZE 8192

namespace icov {
namespace bitstream
{

struct RateStream {
  RateStream()
      : mSyndromeQuantizedSize(0), mSyndromeBitSize(0), mSyndromeByteSize(0) {}

  void reset() {
    mSyndromeQuantizedSize = 0;
    mSyndromeBitSize = 0;
    mSyndromeByteSize = 0;
  }

  void set(byte pSyndromeQuantizedSize,
           const ldpc_codec_layer::IInteractiveCoder &coder, ushort &padding) {

    mSyndromeQuantizedSize = pSyndromeQuantizedSize;

    mSyndromeBitSize =
        bitstream::inverseQuantizedSize(mSyndromeQuantizedSize, coder);

    bitstream::getByteCount(mSyndromeBitSize, mSyndromeByteSize, padding);
  }

  byte syndromeQuantizedSize() const { return mSyndromeQuantizedSize; }
  ushort syndromeBitSize() const { return mSyndromeBitSize; }
  ushort syndromeByteSize() const { return mSyndromeByteSize; }

private:
  byte mSyndromeQuantizedSize;
  ushort mSyndromeBitSize;
  ushort mSyndromeByteSize;
};

struct BitplaneStream {
  NumcodeIdx numcode;
  Syndrome syndrome;
  BitplaneStream() : numcode(0), syndrome(0) {}
};

struct SIEncodedParams {
  void setMode(byte val) { params[0] = val; }
  byte getMode() const { return params[0]; }

  void setPredictionHeader(byte val) { params[1] = val; }
  byte getPredictionHeader() const { return params[1]; }

  void setZmin(byte val) { params[2] = val; }
  byte getZmin() const { return params[2]; }

  void setZmax(byte val) { params[3] = val; }
  byte getZmax() const { return params[3]; }

  BitLength getSyndromeLength() const { return syndromeLength; }
  void setSyndromeLength(BitLength newSyndromeLength) {
    syndromeLength = newSyndromeLength;
  }

  bool getCodingFlag() const { return codingFlag; }
  void setCodingFlag(bool newCodingFlag) { codingFlag = newCodingFlag; }

  void reset() {
    params.fill(0);
    syndromeLength = 0;
    codingFlag = false;
  }

private:
  std::array<byte, 4> params;
  BitLength syndromeLength;
  bool codingFlag;
};

struct SIStream {
  ldpc_codec_layer::QaryDistribution pz;
  byte mode;
  SIVec values;

  std::vector<BitplaneStream> bitplanes;
  int inverseModeQuant() { return mode - 1; }
};

struct BlockChannelDecoderParams {
  icov::byte DirMode;
  icov::byte QIdx;
  icov::byte Zmin;
  icov::byte Zmax;
  //  RawByteArray<icov::byte> BitplaneNumcodes;
  //  RawByteArray<uint16_t> SyndromeBuf;

  std::queue<byte> Bitplanes;

  explicit BlockChannelDecoderParams()
      : DirMode(0), QIdx(0), Zmin(0), Zmax(0) {}
};

struct IcovExtractedBitstream {

  IcovExtractedBitstream()
      : m_WritePosition(0), m_ReadPosition(0), m_ActualSize(0) {
    m_Buffer.fill(0);
  }

  void reset() {
    m_WritePosition = 0;
    m_ReadPosition = 0;
    m_ActualSize = 0;
  }

  void push(const icov::byte &val) {
    auto pos = m_WritePosition;
    icov::assert::check(availableSpace() > 0,
                        ICOV_DEBUG_HEADER("Buffer cannot overflow"));
    shiftWritePosition(1);
    ICOV_TRACE << ">>> EXBT PUSH " << (int)val << " AT " << pos << " NEW SIZE "
               << m_ActualSize;
    m_Buffer[pos] = val;
  }

  icov::byte get() {
    icov::assert::check(!empty(),
                        ICOV_DEBUG_HEADER("Cannot read in empty buffer"));
    ICOV_TRACE << ">>> EXBT READ " << (int)m_Buffer[m_ReadPosition] << " AT "
               << m_ReadPosition;
    return m_Buffer[m_ReadPosition];
  }

  icov::byte pop() {
    byte val = get();
    shiftReadPosition(1);
    return val;
  }

  void get(std::vector<icov::byte> &output) {
    icov::assert::check(m_ActualSize >= output.size(),
                        ICOV_DEBUG_HEADER("Buffer cannot overflow"));
    std::copy(m_Buffer.cbegin() + m_ReadPosition,
              m_Buffer.cbegin() + m_ReadPosition + output.size(),
              output.begin());
  }

  void pop(std::vector<icov::byte> &output) {
    get(output);
    shiftReadPosition(output.size());
  }

  void shiftWritePosition(size_t offset) {
    ICOV_TRACE << __func__ << " " << offset;
    m_WritePosition = (m_WritePosition + offset) % m_Buffer.size();
    m_ActualSize += offset;
    // std::cout << ActualSize << ", " << Buffer.size() << std::endl;

    icov::assert::check_not(
        overflows(), ICOV_DEBUG_HEADER("Buffer write overflow is forbidden"));
    m_ActualSize = std::clamp(m_ActualSize, (size_t)0, m_Buffer.size());
  }

  /**
   * @brief Shifts the read position by a given offset.
   * @param offset The offset by which to shift the read position.
   */
  void shiftReadPosition(size_t offset) {
    ICOV_TRACE << __func__ << " " << offset;
    m_ReadPosition = (m_ReadPosition + offset) % m_Buffer.size();

    icov::assert::check(getActualSize() >= offset,
                        ICOV_DEBUG_HEADER("Buffer read overflow is forbidden"));
    m_ActualSize = m_ActualSize > offset ? m_ActualSize - offset : 0;
  }

  /**
   * @brief Retrieves the current write position in the bitstream.
   * @return The write position.
   */
  size_t getWritePosition() const;

  /**
   * @brief Retrieves the current read position in the bitstream.
   * @return The read position.
   */
  size_t getReadPosition() const;

  /**
   * @brief Retrieves the actual size of the bitstream.
   * @return The actual size.
   */
  size_t getActualSize() const;

  /**
   * @brief Retrieves the full size of the buffer.
   * @return The size of the buffer.
   */
  size_t getBufferSize() const;

  /**
   * @brief Retrieves a pointer to the writable data in the bitstream.
   * @return A pointer to the writable data.
   */
  icov::byte *writableData();

  /**
   * @brief Retrieves a pointer to the readable data in the bitstream.
   * @return A pointer to the readable data.
   */
  const icov::byte *readableData() const;

  /**
   * @brief Checks if the bitstream is empty.
   * @return True if the bitstream is empty, false otherwise.
   */
  bool empty() const { return m_ActualSize == 0; }

  /**
   * @brief Retrieves the number of remaining elements in the bitstream.
   * @return The number of remaining elements.
   */
  int availableSpace() const { return (getBufferSize() - getActualSize()); }

  bool overflows() const { return availableSpace() < 0; }

protected:
  size_t m_WritePosition; /**< The write position in the bitstream. */
  size_t m_ReadPosition;  /**< The read position in the bitstream. */
  size_t m_ActualSize;    /**< The actual size of the bitstream. */
  std::array<icov::byte, BITSTREAM_SIZE>
      m_Buffer; /**< The buffer to store the bitstream data. */
};

using sp_IcovExtractedBitstream = std::shared_ptr<IcovExtractedBitstream>;

inline size_t IcovExtractedBitstream::getWritePosition() const {
  return m_WritePosition;
}

inline size_t IcovExtractedBitstream::getReadPosition() const {
  return m_ReadPosition;
}

inline size_t IcovExtractedBitstream::getActualSize() const {
  return m_ActualSize;
}

inline size_t IcovExtractedBitstream::getBufferSize() const {
  return m_Buffer.size();
}

inline byte *IcovExtractedBitstream::writableData() {
  ICOV_TRACE << __func__;
  return m_Buffer.data() + getWritePosition();
}

inline const byte *IcovExtractedBitstream::readableData() const {
  return m_Buffer.data() + getReadPosition();
}

} // namespace bitstream
} // namespace icov
