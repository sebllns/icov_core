/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup ICOV
 */

/** @file helpers.hpp
 *  @brief BRIEF_DESCRIPTION
 *  DESCRIPTION
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

#include <vector>

namespace icov {
namespace stl_tools {

template <class ForwardIt, class T>
void iota(ForwardIt first, ForwardIt last, T value, T step) {
  while (first != last) {
    *first++ = value;
    value += step;
  }
}

template <class T> std::vector<T> iota(T first, T last, T step) {
  std::vector<T> v;

  if (first < last)
    assert(step > 0);
  if (first > last)
    assert(step < 0);

  T value = first;
  while (value != last) {
    v.push_back(value);
    value += step;
  }
  v.push_back(last);
  return v;
}

template <class T> std::vector<T> iota(T first, T step, int count) {
  return iota(first, first + step * (count - 1), step);
}

template <typename T>
bool lazy_compare_vector(const std::vector<T> &v1, const std::vector<T> &v2) {
  if (v1.size() != v2.size()) {
    return false;
  }

  bool equal = true;
  int i = -1;

  while (equal && ++i < (int)v1.size()) {
    equal = (v1[i] == v2[i]);
  }

  return equal;
}
} // namespace stl_tools
} // namespace icov
