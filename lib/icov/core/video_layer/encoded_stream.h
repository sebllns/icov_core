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

#include "icov_exception.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include "video_layer/ContentParameterSet.h"
#include "video_layer/icov_stream.h"
#include <boost/dynamic_bitset.hpp>
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

inline int
getPredictionOrder(omnidirectional_layer::BlockPrediction::ePredType pred) {

  for (int i = 0; i < PREDICTION_COUNT; ++i)
    if (pred == PredictionID[i])
      return i;

  ICOV_THROW_INVALID_ARGUMENT(
      "Prediction not managed: " +
      omnidirectional_layer::BlockPrediction::toString(pred));

  return PREDICTION_COUNT;
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
