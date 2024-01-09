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

#include "FastArray.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace icov
{

#if !defined(__ANDROID__) && (defined(__linux__) || defined(_WIN32))
typedef uint8_t byte;
typedef uint8_t int8b;
#else
typedef char byte;
typedef char int8b;
#endif

typedef bool bit;
typedef float LLRVal;
typedef int SymboleVal;

/**
 * @typedef BitVal
 * @brief Alias type representing bit values.
 */
typedef int BitVal;

/**
 * @typedef SIVal
 * @brief Alias type for values in a side info.
 */
typedef int SIVal;

/**
 * @typedef SyndVal
 * @brief Alias type for values in a syndrome.
 */
typedef bit SyndVal;

/**
 * @typedef index_t
 * @brief Alias type for indexing value.
 */
typedef uint16_t index_t;

namespace bit_t {
const BitVal zero = 0;
const BitVal one = 1;
} // namespace bit_t

// FastArray is simple memory managed array faster than vector
using Syndrome = FastArray<bit>;
using BitVec = std::vector<BitVal>;
using SymboleVec = std::vector<SymboleVal>;
using LLRVec = std::vector<LLRVal>;
using SIVec = std::vector<SIVal>;
using Bitplanes = std::vector<BitVec>;
using Rate = float;
using Mse = float;
using Psnr = float;
using NumcodeIdx = uint32_t;

/**
 * @typedef BitLength
 * @brief Alias type representing bit lengths.
 */
using BitLength = unsigned long long;

using PixelArea = unsigned long long;

// Bitplane
using SyndromesVec = std::vector<Syndrome>;
using NumcodeIdxVec = std::vector<NumcodeIdx>;

using SINumcodeIdxVec = std::vector<NumcodeIdxVec>;

/**
 * @def shared_ptr(t)
 * @brief A macro for declaring a shared_ptr of type 't'.
 */
#define icov_shared_ptr(t) std::shared_ptr<t>

/**
 * @def unique_ptr(t)
 * @brief A macro for declaring a unique_ptr of type 't'.
 */
#define icov_unique_ptr(t) std::unique_ptr<t>

} // namespace icov
