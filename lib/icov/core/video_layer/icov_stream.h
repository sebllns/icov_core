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
#include "FastArray.h"

#include "icov_assert.h"
#include "ldpc_codec_layer/BitSymbolProvider.h"
#include "ldpc_codec_layer/QaryDistribution.h"
#include "ldpc_codec_layer/interface/IInteractiveCoder.h"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/serialization/access.hpp>
#include <cmath>
#include <cstdint>
#include <sys/types.h>
#include <vector>

#define QARY_PARAMS_SIZE 3

#ifndef ICOV_RATE_SIZE
#define ICOV_RATE_SIZE 1
#endif

namespace icov {
namespace bitstream {

#define BYTE_SIZE CHAR_BIT * sizeof(icov::bitstream::byte)

namespace flag_t {
const byte UnavailableSI = 0;
const byte NoCodingSI = 0;
const byte AccessBlock = 0;
} // namespace flag_t

namespace offset_t {
const byte AvailableSI = 1;
const byte CodingSI = 1;
} // namespace offset_t

namespace enum_t {

enum SyndromeType {
  SyndromeType_NO_CODING = 0,
  SyndromeType_SOURCE = 1,
  SyndromeType_COUNT
};

enum NalUnitType {
  NAL_UNIT_CODED_VPU = 0, // 0
  NAL_UNIT_SOV,           // 1
  NAL_UNIT_CPS,           // 2
  NAL_UNIT_EOC,           // 3

  NAL_UNIT_UNSPECIFIED_001,
  NAL_UNIT_UNSPECIFIED_002,
  NAL_UNIT_UNSPECIFIED_003,
  NAL_UNIT_UNSPECIFIED_004,
  NAL_UNIT_INVALID
};
} // namespace enum_t

inline size_t bitToByteCount(size_t bit_count) {
  return bit_count / BYTE_SIZE + (int)(bit_count % BYTE_SIZE > 0);
}

inline size_t byteToBitCount(size_t byte_count) {
  return BYTE_SIZE * byte_count;
}

inline bool getCodingFlag(byte x) { return x != flag_t::NoCodingSI; }

inline ushort getQaryQParam(byte x) { return x - offset_t::CodingSI; }

inline byte getQaryQParamByteValue(bool coding_flag, ushort x) {
  if (!coding_flag)
    return flag_t::NoCodingSI;
  return x + offset_t::CodingSI;
}

inline bool isPredictionAvailable(byte x) { return x != flag_t::UnavailableSI; }

inline void assertPredictionAvailable(byte x) {
  icov::assert::check(bitstream::isPredictionAvailable(x), "No direction mode for the prediction");
}

inline ushort getPredDirMode(byte x) {
  icov::assert::check(isPredictionAvailable(x), "Prediction must be available");
  return x - offset_t::AvailableSI;
}

inline byte getPredDirModeByteValue(bool available, ushort x) {
  if (!available)
    return flag_t::UnavailableSI;
  return x + offset_t::AvailableSI;
}

// typedef std::basic_streambuf<icov::bitstream::byte> bitstreambuf;

/// Base class for @c icov::bitstream::byte output streams.
typedef std::basic_ostream<icov::bitstream::byte> obitstream;

class bitstreambuf
    : public std::basic_streambuf<icov::bitstream::byte,
                                  std::char_traits<icov::bitstream::byte>> {

public:
  typedef icov::bitstream::byte char_type; // 1
  typedef std::char_traits<icov::bitstream::byte> traits_type;
  typedef typename traits_type::int_type int_type;
  typedef typename traits_type::off_type off_type;
  typedef typename traits_type::pos_type pos_type;
  typedef std::vector<char_type>::size_type size_type;
  using Base = std::basic_streambuf<icov::bitstream::byte>;

  bitstreambuf() : m_buffer(), read_pos(0) {
    // Base::setp(&(*m_buffer.begin()), &(*(m_buffer.end() - 1))); // set
    // std::basic_streambuf
    // put area pointers to work with 'buffer_'
  }

  void seekg(size_type pos) { read_pos = pos; }

  int_type overflow(int_type ch) {
    if (ch != (int_type)EOF)
      m_buffer.push_back(traits_type::to_char_type(ch));
    return ch;
  }

  std::streamsize xsputn(const char_type *s, std::streamsize num) {
    m_buffer.insert(m_buffer.end(), s, s + num);
    return num;
  }

  std::streamsize xsgetn(char_type *s, std::streamsize n) {
    if (read_pos < m_buffer.size()) {
      const size_type num = std::min<size_type>(n, m_buffer.size() - read_pos);
      std::memcpy(s, &m_buffer[read_pos], num);
      read_pos += num;
      return num;
    }
    return 0;
  }

  int size() { return (int)m_buffer.size(); }

private:
  std::vector<char_type> m_buffer;
  size_type read_pos;
};

inline ushort
inverseQuantizedSize(icov::bitstream::byte input,
                     const ldpc_codec_layer::IInteractiveCoder &coder) {
  return (input == enum_t::SyndromeType::SyndromeType_NO_CODING)
             ? (0)
             : ((input == enum_t::SyndromeType::SyndromeType_SOURCE)
                    ? (static_cast<ushort>(coder.get_n()))
                    : (static_cast<ushort>(coder.get_m(input - 2))));
}

inline void getByteCount(int bit_size, ushort &byte_count,
                         ushort &byte_padding) {
  byte_count = icov::bitstream::bitToByteCount(bit_size);
  byte_padding = bit_size % BYTE_SIZE;
}

using rate_t =
#if ICOV_RATE_SIZE == 1
    icov::bitstream::byte;
#elif ICOV_RATE_SIZE == 2
    uint16_t;
#else
    uint32_t;
#endif

typedef icov::bitstream::byte QaryParams[QARY_PARAMS_SIZE];

inline void readQaryParams(const QaryParams input, int &q_idx, int &z_min,
                           int &z_max) {
  q_idx = input[ldpc_codec_layer::QaryDistribution::QaryParamsIndex_Q] -
          offset_t::CodingSI;
  z_min = input[ldpc_codec_layer::QaryDistribution::QaryParamsIndex_Z_MIN];
  z_max = input[ldpc_codec_layer::QaryDistribution::QaryParamsIndex_Z_MAX];

  std::cout << "Q " << (int)input[0] << " " << (int)input[1] << " "
            << (int)input[2] << " -> " << q_idx << " " << z_min << " " << z_max
            << std::endl;
}

inline void writeQaryParams(QaryParams output, int q_idx, int z_min,
                            int z_max) {
  output[ldpc_codec_layer::QaryDistribution::QaryParamsIndex_Q] =
      q_idx + offset_t::CodingSI;
  output[ldpc_codec_layer::QaryDistribution::QaryParamsIndex_Z_MIN] = z_min;
  output[ldpc_codec_layer::QaryDistribution::QaryParamsIndex_Z_MAX] = z_max;

  std::cout << "Q " << (uint)output[0] << " " << (uint)output[1] << " "
            << (uint)output[2] << std::endl;
}

class BitVector {
  typedef icov::bitstream::byte _byte_t;
  typedef bool _bit_t;

public:
  BitVector() = default;

  BitVector(size_t size, _byte_t value = 0) : m_actual_size(size) {
    size_t byte_count =
        m_actual_size / sizeOfByte() + (m_actual_size % sizeOfByte() != 0);

    m_data = std::vector<_byte_t>(byte_count, value);
  }

  static size_t sizeOfByte() { return CHAR_BIT * sizeof(_byte_t); }

  size_t resize(size_t size) {
    m_actual_size = size;

    if (size > max_size()) {
      size_t byte_count =
          m_actual_size / sizeOfByte() + (m_actual_size % sizeOfByte() != 0);
      m_data.resize(byte_count);
    }
    return max_size();
  }
  size_t size() const { return m_actual_size; }
  size_t max_size() const { return byte_count() * sizeOfByte(); }
  size_t byte_count() const { return m_data.size(); }

  _bit_t operator[](int i) const { return get_bit(i); }

  void get_bit_position(int bit_index, int &o_byte_index,
                        int &o_bit_offset) const {
    o_byte_index = bit_index / sizeOfByte();
    o_bit_offset = bit_index % sizeOfByte();
  }

  _byte_t &byte_at(int i) { return m_data[i]; }
  const _byte_t &byte_at(int i) const { return m_data[i]; }

  _byte_t &getByteFromBitPos(int bit_index) {
    int byte_index = 0;
    int bit_offset = 0;
    get_bit_position(bit_index, byte_index, bit_offset);
    return this->m_data[byte_index];
  }

  _byte_t getByteFromBitPos(int bit_index) const {
    int byte_index = 0;
    int bit_offset = 0;
    get_bit_position(bit_index, byte_index, bit_offset);
    return this->m_data[byte_index];
  }

  _bit_t get_bit(int bit_index) const {
    int byte_index = 0;
    int bit_offset = 0;
    get_bit_position(bit_index, byte_index, bit_offset);

    return (this->m_data[byte_index] >> bit_offset) & 1;
  }

  float get_bit_value(int bit_index) const {
    return (get_bit(bit_index)) ? 1.0 : 0.0;
  }

  void set(int bit_index, _bit_t bit_val) {
    int byte_index = 0;
    int bit_offset = 0;
    get_bit_position(bit_index, byte_index, bit_offset);

    int mask = 1 << bit_offset;

    this->m_data[byte_index] =
        ((this->m_data[byte_index] & ~mask) | ((_byte_t)bit_val << bit_offset));
  }

  void print() const {
    for (size_t i = 0; i < m_actual_size; i++) {
      std::cout << get_bit(i) << " ";
    }
    std::cout << std::endl;
  }

protected:
  size_t m_actual_size;
  std::vector<_byte_t> m_data;
};

template <typename T>
icov::bitstream::byte boolToByte(const std::array<T, 8> in) {
  icov::bitstream::byte result = 0;
  int i = 0;

  for (auto &v : in) {
    result = (result << 1);
    if (static_cast<bool>(in[i++]))
      result++;
  }
  return result;
}

inline bool getBit(int bit_pos, byte b) { return (b >> bit_pos) & 1; }

inline void setBit(int bit_pos, byte &b) { b |= 1 << bit_pos; }

} // namespace bitstream
} // namespace icov
