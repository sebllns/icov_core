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
#include "common.h"

#include <cstdint>

namespace icov {
/** @class ContentParameterSet
 *  @brief class ContentParameterSet
 *
 *  Conveys the video parameters part of bitstream.
 */
struct ContentParameterSet {
  std::string icov_version;

  icov::byte content_type;

  uint16_t content_width;
  uint16_t content_height;

  uint16_t qp;

  icov::byte content_chroma_format;

  int content_framecount;
  float content_framerate;
  float content_duration;
  float viewport_padding;

  bool is_sov_fixed_size;
  icov::byte sov_fixed_size;

  bool is_block_fixed_size;
  icov::byte block_fixed_size;

  bool is_tu_count_fixed;
  icov::byte fixed_tu_count;

  icov::byte numcode_byte_count;

  bool has_metadata;

  ContentParameterSet()
      : content_type(0), content_width(0), content_height(0), qp(42),
      content_chroma_format(0), content_framecount(0), content_framerate(0),
      content_duration(0), viewport_padding(1.2), is_sov_fixed_size(true),
        sov_fixed_size(1), is_block_fixed_size(true), block_fixed_size(5),
        is_tu_count_fixed(true), fixed_tu_count(1), numcode_byte_count(1),
        has_metadata(false) {}
};
} // namespace icov
