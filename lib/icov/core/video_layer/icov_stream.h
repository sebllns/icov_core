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
#include "BitSymbolProvider.h"
#include "helpers.h"
#include "typedef.h"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/serialization/access.hpp>
#include <cmath>
#include <cstdint>
#include <sys/types.h>
#include <vector>

#define QARY_PARAMS_SIZE 3

namespace icov {
namespace bitstream {

// ERRORS : NEED 1 BYTE FOR ZMIN AND 1 BYTE FOR ZMAX
// WHY ZMIN AND ZMAX ARE 0 ??
typedef icov::byte QaryParams[QARY_PARAMS_SIZE];

inline void readQaryParams(const QaryParams input, int &q_idx, int &z_min,
                           int &z_max) {
  // lx = input[0] >> 4;
  q_idx = input[0] - 1; // & 15;

  z_min = input[1] * Z_STEP - Z_LIMIT;
  z_max = input[2] * Z_STEP - Z_LIMIT;
}

inline void writeQaryParams(QaryParams output, int q_idx, int z_min,
                            int z_max) {

  // output[0] = lx << 4;
  output[0] = q_idx + 1;

  output[1] = ((z_min + Z_LIMIT) / Z_STEP);
  output[2] = ((z_max + Z_LIMIT) / Z_STEP);

  std::cout << "BSP " << q_idx << ", " << z_min << ", " << z_max << "->"
            << (uint)output[0] << ", " << (uint)output[1] << ", "
            << (uint)output[2] << std::endl;
}

} // namespace bitstream
} // namespace icov
