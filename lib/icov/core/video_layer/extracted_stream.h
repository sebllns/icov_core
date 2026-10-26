/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup ICOV
 */

/** @file ContentParameterSet.h
 *  @brief ContentParameterSet
 *  @ingroup common
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

#include "icov_assert.h"
#include "icov_exception.h"
#include "icov_types.h"
#include "ldpc_codec_layer/QaryDistribution.h"
#include "ldpc_codec_layer/interface/IInteractiveCoder.h"
#include "video_layer/bit_io.h"
#include "video_layer/icov_stream.h"

#include <array>
#include <boost/dynamic_bitset.hpp>

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <queue>
#include <sstream>
#include <sys/types.h>
#include <vector>


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

  std::string toString() {
    std::stringstream ss;
    ss << "qsize=" << (ushort)mSyndromeQuantizedSize
       << ", bsize=" << mSyndromeBitSize << ", Bsize=" << mSyndromeByteSize;
    return ss.str();
  }

private:
  byte mSyndromeQuantizedSize;
  ushort mSyndromeBitSize;
  ushort mSyndromeByteSize;
};

struct BitplaneStream {
  NumcodeIdx numcode;
  Syndrome syndrome;
  BitplaneStream() : numcode(255), syndrome(0) {}
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

  bool getEnabledFlag() const { return enabledFlag; }
  void setEnabledFlag(bool newenabledFlag) { enabledFlag = newenabledFlag; }

  void reset() {
    params.fill(0);
    syndromeLength = 0;
    codingFlag = false;
    enabledFlag = true;
  }

private:
  std::array<byte, 4> params;
  BitLength syndromeLength;
  bool codingFlag;
  bool enabledFlag;
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

/// Transmitted block unit (TBU): what the extractor (server role) sends to
/// the decoder (client role) for one block, at bit level. Per channel:
///   num_bitplanes                         ue(v)
///   if not an access block:
///     si_mode                             u(7)   intra direction + 1
///     pred_header                         ue(v)  q index + 1, 0: not coded
///     if coded: z_min, z_max              u(8)
///     if coded: rate index per bitplane   ue(v)
///   syndrome prefix per bitplane          exact bit length, no padding
/// then byte_alignment() once per block. Lengths are implicit: the decoder
/// derives each syndrome length from its rate index.
class IcovExtractedBitstream {
public:
  // writer side (extractor)
  void writeBitplaneCount(unsigned v) { m_writer.ue(v); }
  void writeMode(unsigned v) { m_writer.u(7, v); }
  void writePredHeader(unsigned v) { m_writer.ue(v); }
  void writeZ(unsigned v) { m_writer.u(8, v); }
  void writeRate(unsigned v) { m_writer.ue(v); }
  void writeSyndrome(const uint8_t *data, size_t nbits) {
    m_writer.bits(data, nbits);
  }
  /// Ends the unit of one block
  void endUnit() { m_writer.byte_alignment(); }
  size_t unitBytes() const { return m_writer.data().size(); }

  // reader side (decoder)
  unsigned readBitplaneCount() { return reader().ue(); }
  unsigned readMode() { return reader().u(7); }
  unsigned readPredHeader() { return reader().ue(); }
  unsigned readZ() { return reader().u(8); }
  unsigned readRate() { return reader().ue(); }
  bool readSyndromeBit() { return reader().flag(); }

  /// True when no whole byte is left to read
  bool isEmpty() const {
    return m_reading ? m_reader.bits_left() < 8 : m_writer.bit_count() == 0;
  }
  size_t getActualSize() const { return m_writer.data().size(); }

  void empty() {
    m_writer.clear();
    m_reader.reset(nullptr, 0);
    m_reading = false;
  }

private:
  BitReader &reader() {
    if (!m_reading) {
      m_reader.reset(m_writer.data().data(), m_writer.data().size());
      m_reading = true;
    }
    return m_reader;
  }

  BitWriter m_writer;
  BitReader m_reader;
  bool m_reading = false;
};

using sp_IcovExtractedBitstream = std::shared_ptr<IcovExtractedBitstream>;
} // namespace bitstream
} // namespace icov
