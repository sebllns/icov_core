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
#include "helpers.h"
#include "icov_stream.h"
#include "typedef.h"

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/queue.hpp>
#include <boost/serialization/vector.hpp>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <sys/types.h>
#include <vector>

namespace icov {

// struct BlockBitplaneDecoderParams {
//  icov::byte Numcode;
//  BlockBitplaneDecoderParams() : Numcode(0), SyndromeBuf(nullptr) {}
//};

template <typename T> struct RawByteArray : boost::noncopyable {
  T Length;
  icov::byte *Data;
  void release() {
    Length = 0;
    if (Data != nullptr)
      delete[] Data;
  }
  RawByteArray() : Length(0), Data(nullptr) {}
  ~RawByteArray() {}
};

struct BlockChannelDecoderParams {
  icov::byte DirMode;
  bool CloneSI;
  icov::byte QIdx;
  icov::byte Zmin;
  icov::byte Zmax;
  RawByteArray<icov::byte> BitplaneNumcodes;
  RawByteArray<uint16_t> SyndromeBuf;
  explicit BlockChannelDecoderParams()
      : DirMode(0), CloneSI(false), QIdx(0), Zmin(0), Zmax(0) {}
};

} // namespace icov

namespace boost {
namespace serialization {

template <class Archive, typename T>
void serialize(Archive &ar, icov::RawByteArray<T> &io,
               const unsigned int /*version*/) {
  ar &io.Length;
  if (Archive::is_loading::value) {
    io.release();
    io.Data = new icov::byte[io.Length];
  }
  ar &make_array<icov::byte>(io.Data, io.Length);
}

template <class Archive>
void serialize(Archive &ar, icov::BlockChannelDecoderParams &io,
               const unsigned int /*version*/) {

  ar &io.DirMode;
  ar &io.CloneSI;

  if (!io.CloneSI) {
    ar &io.BitplaneNumcodes;
    if (io.BitplaneNumcodes.Length > 0) {
      ar &io.QIdx;
      ar &io.Zmin;
      ar &io.Zmax;
    }
    ar &io.SyndromeBuf;
  }

  //  size_t byte_count = bitToByteCount(io.size());
  //  icov::byte *data = new icov::byte[byte_count];
  //  ar &make_array<icov::byte>(data, byte_count);
}

} // namespace serialization
} // namespace boost
