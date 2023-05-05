/****************************************************************************/
/** @addtogroup Video
 *  Video contains the code related to video compression.
 *  @ingroup ICOV
 */

/** @file video_common.h
 *  @brief video_common
 *  @ingroup Video
 *  @author Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.0.0
 *  @date 2018, 2022
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
#include "CommonLib/Common.h"
#include "CommonLib/TypeDef.h"
#include "common.h"
#include <CommonLib/Buffer.h>
#include <CommonLib/Unit.h>

namespace icov {
namespace video {

using Bool = bool;
using Int = int;
using UInt = unsigned int;
using Void = void;
using Int64 = long long;
using UInt64 = unsigned long long;
using UChar = unsigned char; // Unsigned 8-bit values

typedef signed char SChar; // Signed 8-bit values

// typedef       unsigned int        UInt;
// typedef       int                 Int;
// typedef       unsigned char       UChar; // Unsigned 8-bit values
// typedef       void                void;
// typedef       bool                Bool;
// typedef       long long           Int64;
// typedef       unsigned long long  UInt64;

static const Int chromaQPMappingTableSize = 58;
// extern const UChar
// g_aucChromaScale[NUM_CHROMA_FORMAT][chromaQPMappingTableSize];

inline Int getScaledChromaQP(Int unscaledChromaQP, const ChromaFormat chFmt) {
  return 0;
  // return g_aucChromaScale[chFmt][Clip3(0, (chromaQPMappingTableSize - 1),
  // unscaledChromaQP)];
}

static const Int ADJ_QUANT_SHIFT = 7;
static const Int ADJ_DEQUANT_SHIFT = (ADJ_QUANT_SHIFT + 1);
static const Int MAX_TU_SIZE = 128;
extern SChar g_aucLog2[MAX_CU_SIZE + 1];

void init_video_lib(int argc, char *argv[]);
} // namespace video
} // namespace icov
