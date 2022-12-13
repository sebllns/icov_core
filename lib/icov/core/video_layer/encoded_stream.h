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
 *  @version 1.0.1
 *  @date 2022
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
#include "encoded_stream/ContentParameterSet.h"
#include "icov_stream.h"
#include "typedef.h"
#include <boost/dynamic_bitset.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/array_wrapper.hpp>
#include <boost/serialization/serialization.hpp>
#include <cmath>
#include <cstdint>
#include <sys/types.h>
#include <vector>

// template <typename T>
// void serialize(std::vector<icov::byte> &output, const std::vector<T> &input,
//               int bit_length, bool is_signed) {

//  icov::helpers::assert::check(
//      bit_length >= CHAR_BIT * sizeof(icov::byte),
//      "Use this method only to convert data shorter than bytes");

//  int byte_length = std::floor(bit_length /) + 1;

//  icov::byte value = 0;

//  if (is_signed && input < 0) {
//    value = 1;
//  }

//  for (int i = 0; i < bit_length; ++i) {
//    value << 1;
//  }
//}

using namespace icov;

inline int read_syndrome(Syndrome &s, const icov::byte *data, int offset = 0) {
  int bit_shift = 0;
  for (size_t i = 0; i < s.size(); ++i) {
    size_t bit_idx = i + offset;
    bit_shift = BYTE_SIZE - bit_idx % BYTE_SIZE - 1;
    size_t byte_idx = bit_idx / BYTE_SIZE;
    s[i] = icov::helpers::bitstream::getBit(bit_shift, data[byte_idx]);
  }
  return bit_shift;
}

inline int write_syndrome(icov::byte *data, const Syndrome &s, int offset = 0) {
  // data = (icov::byte *)calloc(s.size(), sizeof(icov::byte));
  int bit_shift = 0;
  for (size_t i = 0; i < s.size(); ++i) {
    size_t bit_idx = i + offset;
    bit_shift = BYTE_SIZE - bit_idx % BYTE_SIZE - 1;
    size_t byte_idx = bit_idx / BYTE_SIZE;
    data[byte_idx] += s[i] << (bit_shift);
  }
  return bit_shift; // then if != 0, continue from this, then calculate new
                    // size bytesize(bit_size + bit_shift)
}

namespace boost {
namespace serialization {

// When the class Archive corresponds to an output archive, the
// & operator is defined similar to <<.  Likewise, when the class Archive
// is a type of input archive the & operator is defined similar to >>.

template <class Archive>
void serialize(Archive &ar, Syndrome &io, const unsigned int version) {
  size_t byte_count = bitToByteCount(io.size());
  icov::byte *data = new icov::byte[byte_count];
  ar &make_array<icov::byte>(data, byte_count);
}

template <class Archive>
void serialize(Archive &ar, ContentParameterSet &io,
               const unsigned int version) {

  ar &io.icov_version;
  ar &io.content_type;
  ar &io.content_width;
  ar &io.content_height;
  ar &io.content_chroma_format;
  ar &io.content_framecount;
  ar &io.content_framerate;
  ar &io.content_duration;

  ar &io.is_sov_fixed_size;
  if (io.is_sov_fixed_size)
    ar &io.sov_fixed_size;

  ar &io.is_block_fixed_size;
  if (io.is_block_fixed_size)
    ar &io.block_fixed_size;

  ar &io.is_tu_count_fixed;
  if (io.is_tu_count_fixed)
    ar &io.fixed_tu_count;

  ar &io.has_metadata;
}

template <typename Ar, typename Block, typename Alloc>
void save(Ar &ar, dynamic_bitset<Block, Alloc> const &bs, unsigned) {
  size_t num_bits = bs.size();
  std::vector<Block> blocks(bs.num_blocks());
  to_block_range(bs, blocks.begin());

  ar &num_bits &blocks;
}

template <typename Ar, typename Block, typename Alloc>
void load(Ar &ar, dynamic_bitset<Block, Alloc> &bs, unsigned) {
  size_t num_bits;
  std::vector<Block> blocks;
  ar &num_bits &blocks;

  bs.resize(num_bits);
  from_block_range(blocks.begin(), blocks.end(), bs);
  bs.resize(num_bits);
}

template <typename Ar, typename Block, typename Alloc>
void serialize(Ar &ar, dynamic_bitset<Block, Alloc> &bs, unsigned version) {
  split_free(ar, bs, version);
}

} // namespace serialization
} // namespace boost
