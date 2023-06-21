/****************************************************************************/
/** @addtogroup ICOV
 * ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup ICOV
 */

/** @file common.h
 *  @brief Defines version information, constants and small in-line functions
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

#ifndef COMMON_H
#define COMMON_H

#include "icov_sys.h"

#include "icov_assert.h"
#include "icov_console.h"
#include "icov_debug.h"
#include "icov_exception.h"
#include "icov_io.h"
#include "icov_log.h"
#include "icov_math.h"
#include "icov_omp.h"
#include "icov_path.h"
#include "icov_stacktrace.h"
#include "icov_stl_tools.h"
#include "version.h"

#include "FastArray.h"

#include <algorithm>
#include <cstdio>
#include <ctype.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale.h>
#include <locale>
#include <math.h>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>

namespace icov {

typedef bool bit;
typedef float LLRVal;
typedef int SymboleVal;
typedef int BitVal;
typedef int SIVal;
typedef bit SyndVal;
typedef uint16_t index_t;

namespace bit_t {
const BitVal zero = 0;
const BitVal one = 1;
} // namespace bit_t

// FastArray is faster
using Syndrome = FastArray<bit>;
using BitVec = std::vector<BitVal>;
using SymboleVec = std::vector<SymboleVal>;
using LLRVec = std::vector<LLRVal>;
using SIVec = std::vector<SIVal>;
using Bitplanes = std::vector<BitVec>;
using Rate = float;
using NumcodeIdx = uint32_t;

// Bitplane
using SyndromesVec = std::vector<Syndrome>;
using NumcodeIdxVec = std::vector<NumcodeIdx>;

using SINumcodeIdxVec = std::vector<NumcodeIdxVec>;

} // namespace icov

#endif // COMMON_H
