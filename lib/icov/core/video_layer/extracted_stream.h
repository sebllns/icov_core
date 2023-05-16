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
#include "helpers.h"
#include "icov_stream.h"
#include "ldpc_codec_layer/BitSymbolProvider.h"
#include "omnidirectional_layer/Block.h"
#include <algorithm>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/queue.hpp>
#include <boost/serialization/vector.hpp>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <sys/types.h>
#include <vector>

#define BITSTREAM_SIZE 8192

namespace icov {
namespace bitstream {

// struct BlockBitplaneDecoderParams {
//  icov::bitstream::byte Numcode;
//  BlockBitplaneDecoderParams() : Numcode(0), SyndromeBuf(nullptr) {}
//};

template <typename T> struct RawByteArray : boost::noncopyable {
  T Length;
  icov::bitstream::byte *Data;
  void release() {
    Length = 0;
    if (Data != nullptr)
      delete[] Data;
  }
  void reset(T l) {
    release();
    Length = l;
    Data = new icov::bitstream::byte[l];
  }
  explicit RawByteArray(T l) : Length(l), Data(new icov::bitstream::byte[l]) {}
  RawByteArray() : Length(0), Data(nullptr) {}
  ~RawByteArray() {}
};

struct BitplaneStream {
  NumcodeIdx numcode;
  Syndrome syndrome;
};

struct SIStream {
  ldpc_codec_layer::QaryDistribution pz;
  byte mode;

  std::vector<BitplaneStream> bitplanes;
  int inverseModeQuant() { return mode - 1; }
};

struct BlockChannelDecoderParams {
  icov::bitstream::byte DirMode;
  icov::bitstream::byte QIdx;
  icov::bitstream::byte Zmin;
  icov::bitstream::byte Zmax;
  //  RawByteArray<icov::bitstream::byte> BitplaneNumcodes;
  //  RawByteArray<uint16_t> SyndromeBuf;

  std::queue<icov::bitstream::byte> Bitplanes;

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

  void push(const icov::bitstream::byte &val) {
    auto pos = m_WritePosition;
    shiftWritePosition(1);
    std::cout << "PUSH " << (int)val << " AT " << pos << " NEW SIZE "
              << m_ActualSize << std::endl;
    m_Buffer[pos] = val;
  }

  icov::bitstream::byte pop() {
    icov::helpers::assert::check(m_ActualSize > 0, "Buffer is not empty");
    auto pos = m_ReadPosition;
    shiftReadPosition(1);
    return m_Buffer[pos];
  }

  void shiftWritePosition(size_t offset) {
    m_WritePosition = (m_WritePosition + offset) % m_Buffer.size();
    m_ActualSize += offset;
    // std::cout << ActualSize << ", " << Buffer.size() << std::endl;
    ICOV_WARNING_MSG(m_ActualSize <= m_Buffer.size(),
                     "Buffer overflow is forbidden");
    m_ActualSize = std::clamp(m_ActualSize, (size_t)0, m_Buffer.size());
  }

  void shiftReadPosition(size_t offset) {
    m_ReadPosition = (m_ReadPosition + offset) % m_Buffer.size();
    ICOV_WARNING_MSG(m_ActualSize >= offset,
                     "Buffer negative overflow is forbidden");
    m_ActualSize = m_ActualSize > offset ? m_ActualSize - offset : 0;
  }

  size_t getWritePosition() const;
  size_t getReadPosition() const;
  size_t getActualSize() const;
  icov::bitstream::byte *writableData();
  const icov::bitstream::byte *readableData() const;
  bool empty() const { return m_ActualSize > 0; }
  int remains() const { return (m_WritePosition - m_ReadPosition); }

protected:
  size_t m_WritePosition;
  size_t m_ReadPosition;
  size_t m_ActualSize;
  std::array<icov::bitstream::byte, BITSTREAM_SIZE> m_Buffer;
};

// using IcovExtractedBitstream = std::vector<icov::bitstream::byte>;
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

inline byte *IcovExtractedBitstream::writableData() {
  return m_Buffer.data() + getWritePosition();
}

inline const byte *IcovExtractedBitstream::readableData() const {
  return m_Buffer.data() + getReadPosition();
}

} // namespace bitstream
} // namespace icov

namespace boost {
namespace serialization {

template <class Archive, typename T>
void serialize(Archive &ar, icov::bitstream::RawByteArray<T> &io,
               const unsigned int /*version*/) {
  ar &io.Length;
  if (Archive::is_loading::value) {
    io.release();
    io.Data = new icov::bitstream::byte[io.Length];
  }
  ar &make_array<icov::bitstream::byte>(io.Data, io.Length);
}

template <class Archive>
void serialize(Archive &ar, icov::bitstream::BlockChannelDecoderParams &io,
               const unsigned int /*version*/) {

  ar &io.DirMode;

  if (io.DirMode == 0) {
    // ACCESS BLOCK
    ar &io.Bitplanes;
    ICOV_ASSERT_MSG(io.Bitplanes.size() > 0, "No access block data");
  } else {
    // NON ACCESS BLOCK
    ar &io.QIdx;
    if (io.QIdx > 0) {
      ar &io.Bitplanes;
      if (io.Bitplanes.size() > 0) {
        ar &io.Zmin;
        ar &io.Zmax;
        // ar &io.SyndromeBuf;
      } // ELSE NO BITPLANE => 0
    }   // else CLONE SI
  }
  //  size_t byte_count = bitToByteCount(io.size());
  //  icov::bitstream::byte *data = new icov::bitstream::byte[byte_count];
  //  ar &make_array<icov::bitstream::byte>(data, byte_count);
}

} // namespace serialization
} // namespace boost
