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
#include "common_types.h"
#include "extracted_stream/NalUnit.h"
#include "extracted_stream/ViewportUnit.h"
#include "icov_stream.h"
#include "typedef.h"

#include <boost/dynamic_bitset.hpp>
#include <boost/serialization/access.hpp>
#include <cmath>
#include <cstdint>
#include <sys/types.h>
#include <vector>

namespace icov {

struct BlockDecoderParams {
  icov::byte DirMode;
  bool CloneSI;
  icov::byte LX;
  icov::byte QIdx;
  icov::byte Zmin;
  icov::byte Zmax;
  icov::byte Numcode;

  explicit BlockDecoderParams()
      : DirMode(0), CloneSI(false), LX(0), QIdx(0), Zmin(0), Zmax(0),
        Numcode(0) {}
};

} // namespace icov

namespace boost {
namespace serialization {

template <class Archive>
void serialize(Archive &ar, icov::BlockDecoderParams &io,
               const unsigned int /*version*/) {

  ar &io.DirMode;
  ar &io.CloneSI;
  ar &io.LX;
  ar &io.QIdx;
  ar &io.Zmin;
  ar &io.Zmax;
  ar &io.Numcode;

  //  size_t byte_count = bitToByteCount(io.size());
  //  icov::byte *data = new icov::byte[byte_count];
  //  ar &make_array<icov::byte>(data, byte_count);
}

} // namespace serialization
} // namespace boost
