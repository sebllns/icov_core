/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup ICOV
 */

/** @file typeDef.h
 *  @brief Defines macros, basic types, new types and enumerations.
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

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <vector>

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES // for C++
#endif

#define RAD2DEG (180.0 * M_1_PI)
#define DEG2RAD (M_PI / 180.0)

namespace icov {
namespace maths {

int pow2(int k);

/**
 * @brief Computes the binary length of a given integer value.
 * @param val The integer value to compute the binary length for.
 * @return The binary length of the given integer value.
 */
int pow2(int k) { return 1 << k; }

uint32_t binary_length(int val) {
  return std::floor(std::log2f(std::abs(val))) + 1;
}

float normalize(float val, float min, float max) {
  return (val - min) / (max - min);
}

float wrap(float val, float min, float max) {
  while (val < min) {
    val += (max - min);
  }

  while (val > max) {
    val -= (max - min);
  }

  return val;

  // return val - (min - max) * floor(val / (max - min));
}

int quantize(int value, int step, int min, int max) {
  return value == 0
             ? 0
             : std::clamp((int)((std::abs(value) / value) * step *
                                std::ceil((1.0 * std::abs(value)) / step)),
                          min, max);
}

int quantize(float value, const std::vector<float> &v) {
  int idx = std::upper_bound(v.begin(), v.end(), value) - v.begin();
  idx = std::clamp(idx, 0, (int)v.size() - 1);
  int idx_prev = std::clamp(idx - 1, 0, (int)v.size() - 1);

  if (std::abs(value - v[idx_prev]) < std::abs(value - v[idx])) {
    idx = idx_prev;
  }
  return idx;
}

namespace rand {

void init() { srand((unsigned)time(0)); }

int range(int min_included, int max_included) {
  return min_included +
         (std::rand() % static_cast<int>(max_included - min_included + 1));
}

float normalized() {
  return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

} // namespace rand
} // namespace maths
} // namespace icov
