/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup ICOV
 */

/** @file encoded_stream.h
 *  @brief Contains various utilities and types related to bitstream
 *manipulation.
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

#include "icov_exception.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include "video_layer/ContentParameterSet.h"
#include "video_layer/icov_stream.h"
#include <boost/dynamic_bitset.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/array_wrapper.hpp>
#include <boost/serialization/serialization.hpp>
#include <cmath>
#include <cstdint>
#include <sys/types.h>
#include <vector>

#define PREDICTION_COUNT 9
#define ACCESS_BLOCK_PREDICTION_COUNT 1
#define TOTAL_PREDICTION_COUNT                                                 \
  (PREDICTION_COUNT + ACCESS_BLOCK_PREDICTION_COUNT)

namespace icov {

namespace bitstream {

typedef enum {
  MEDIA_HEADER = 0,
  FRAMES_SIZE,
  BLOCKS_SIZE,
  ENCODE_PARAMS,
  SYNDROME,
  METRICS,
  COUNT
} eIcovStream;

static const omnidirectional_layer::IBlockPredictionManager::prediction_t
    PredictionID[PREDICTION_COUNT] = {
        omnidirectional_layer::BlockPrediction::INTER,
        omnidirectional_layer::BlockPrediction::TOP_RIGHT,
        omnidirectional_layer::BlockPrediction::TOP_LEFT,
        omnidirectional_layer::BlockPrediction::BOTTOM_RIGHT,
        omnidirectional_layer::BlockPrediction::BOTTOM_LEFT,
        omnidirectional_layer::BlockPrediction::RIGHT,
        omnidirectional_layer::BlockPrediction::LEFT,
        omnidirectional_layer::BlockPrediction::TOP,
        omnidirectional_layer::BlockPrediction::BOTTOM,
};

inline std::string getIcovPartName(int id) {
  switch (static_cast<eIcovStream>(id)) {
  case MEDIA_HEADER:
    return "header";
  case FRAMES_SIZE:
    return "fsize";
  case BLOCKS_SIZE:
    return "bsize";
  case ENCODE_PARAMS:
    return "prediction";
  case SYNDROME:
    return "syndrome";
  case METRICS:
    return "metric";
  default:
    ICOV_THROW_INVALID_ARGUMENT("getIcovPartName");
  }
  return "";
}

inline int
getPredictionOrder(omnidirectional_layer::BlockPrediction::ePredType pred) {

  switch (pred) {

  case omnidirectional_layer::BlockPrediction::INTER:
    return 0;

  case omnidirectional_layer::BlockPrediction::TOP_RIGHT:
    return 1;

  case omnidirectional_layer::BlockPrediction::TOP_LEFT:
    return 2;

  case omnidirectional_layer::BlockPrediction::BOTTOM_RIGHT:
    return 3;

  case omnidirectional_layer::BlockPrediction::BOTTOM_LEFT:
    return 4;

  case omnidirectional_layer::BlockPrediction::RIGHT:
    return 5;

  case omnidirectional_layer::BlockPrediction::LEFT:
    return 6;

  case omnidirectional_layer::BlockPrediction::TOP:
    return 7;

  case omnidirectional_layer::BlockPrediction::BOTTOM:
    return 8;

  default:
    ICOV_THROW_INVALID_ARGUMENT("Unrecognized prediction value");
  }
  return -1;
}

template <typename T>
int read_bits(T &output, const icov::byte *input, int offset = 0) {
  int bit_shift = 0;
  for (size_t i = 0; i < output.size(); ++i) {
    size_t bit_idx = i + offset;
    bit_shift = BYTE_SIZE - bit_idx % BYTE_SIZE - 1;
    size_t byte_idx = bit_idx / BYTE_SIZE;
    output[i] = icov::bitstream::getBit(bit_shift, input[byte_idx]);
  }
  return bit_shift;
}

template <typename T>
int write_bits(const T &input, icov::byte *output, int offset = 0) {
  uint b = offset / BYTE_SIZE;
  uint e = b + input.size() / BYTE_SIZE;
  std::fill(output + b, output + e, 0);
  int bit_shift = 0;
  for (size_t i = 0; i < input.size(); ++i) {
    size_t bit_idx = i + offset;
    bit_shift = BYTE_SIZE - bit_idx % BYTE_SIZE - 1;
    if (input[i]) {
      size_t byte_idx = bit_idx / BYTE_SIZE;
      output[byte_idx] |= 1 << (bit_shift);
    }
  }
  return (BYTE_SIZE - bit_shift) %
         BYTE_SIZE; // then if != 0, continue from this, then calculate new
                    // size bytesize(bit_size + bit_shift)
}

inline std::string writeSyndrome(const Syndrome &s) {
  const int byte_size = bitstream::bitToByteCount(s.size());

  if (byte_size == 0)
    return "void";

  icov::byte *bytes = new icov::byte[byte_size];
  bitstream::write_bits(s, bytes);

  std::string res =
      icov::io::make_hex_string(bytes, bytes + byte_size, true, true);

  delete[] bytes;
  bytes = nullptr;
  return res;
}

} // namespace bitstream
} // namespace icov
namespace boost {
namespace serialization {

// When the class Archive corresponds to an output archive, the
// & operator is defined similar to <<.  Likewise, when the class Archive
// is a type of input archive the & operator is defined similar to >>.

template <class Archive>
void serialize(Archive &ar, icov::Syndrome &io,
               const unsigned int /*version*/) {
  size_t byte_count = icov::bitstream::bitToByteCount(io.size());
  icov::byte *data = new icov::byte[byte_count];
  ar &make_array<icov::byte>(data, byte_count);
}

template <class Archive>
void serialize(Archive &ar, icov::bitstream::ContentParameterSet &io,
               const unsigned int /*version*/) {

  ar &io.icov_version;
  ar &io.content_type;
  ar &io.content_width;
  ar &io.content_height;
  ar &io.qp;
  ar &io.content_chroma_format;
  ar &io.content_framecount;
  ar &io.content_framerate;
  ar &io.content_duration;
  ar &io.viewport_padding;
  ar &io.viewport_fov_x;
  ar &io.viewport_fov_y;
  ar &io.viewport_height;
  ar &io.viewport_width;

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
  ar &io.icu_count;
  ar &io.encoded_blocks;
}

template <typename Ar, typename Block, typename Alloc>
void save(Ar &ar, dynamic_bitset<Block, Alloc> const &bs, unsigned) {
  size_t num_bits = bs.size();
  std::vector<Block> blocks(bs.num_blocks());
  to_block_range(bs, blocks.begin());

  ar &num_bits &blocks;
}

template <typename Ar, typename Block, typename Alloc>
void load(Ar &ar, dynamic_bitset<Block, Alloc> &bs, unsigned /*version*/) {
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
