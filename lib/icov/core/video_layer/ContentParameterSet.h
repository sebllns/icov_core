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

#include "omnidirectional_layer/Block.h"
#include "video_layer/icov_stream.h"
#include <cstdint>

namespace icov {
namespace bitstream {

/** @class ContentParameterSet
 *  @brief class ContentParameterSet
 *
 *  Conveys the video parameters part of bitstream.
 */
struct ContentParameterSet {
  std::string icov_version;

  byte content_type;
  uint16_t content_width;
  uint16_t content_height;

  uint16_t viewport_width, viewport_height;
  float viewport_fov_x, viewport_fov_y;

  byte qp;
  uint16_t ldpc_bp_ite;
  std::string ldpc_coder_type;

  byte content_chroma_format;

  int content_framecount;
  float content_framerate;
  float content_duration;
  float viewport_padding;

  bool is_sov_fixed_size;
  byte sov_fixed_size;

  bool is_block_fixed_size;
  byte block_fixed_size;

  bool is_tu_count_fixed;
  byte fixed_tu_count;

  byte numcode_byte_count;

  bool has_metadata;

  uint16_t icu_count;
  std::map<omnidirectional_layer::Block::id_t,
           omnidirectional_layer::Block::id_t>
      encoded_blocks;

  //@todo TODO q_values in ContentParameterSet
  std::vector<float> q_values;

  ContentParameterSet()
      : content_type(0), content_width(0), content_height(0), viewport_width(0),
        viewport_height(0), viewport_fov_x(0), viewport_fov_y(0), qp(42),
        ldpc_bp_ite(100), ldpc_coder_type("imt164"), content_chroma_format(0),
        content_framecount(0), content_framerate(0), content_duration(0),
        viewport_padding(1.2), is_sov_fixed_size(true), sov_fixed_size(1),
        is_block_fixed_size(true), block_fixed_size(5), is_tu_count_fixed(true),
        fixed_tu_count(1), numcode_byte_count(1), has_metadata(false),
        icu_count(0) {}
};

inline bool operator==(const ContentParameterSet &a,
                       const ContentParameterSet &b) {

  return a.icov_version == b.icov_version && a.content_type == b.content_type &&
         a.content_width == b.content_width &&
         a.content_height == b.content_height &&
         a.viewport_width == b.viewport_width &&
         a.viewport_height == b.viewport_height &&
         a.viewport_fov_x == b.viewport_fov_x &&
         a.viewport_fov_y == b.viewport_fov_y && a.qp == b.qp &&
         a.content_chroma_format == b.content_chroma_format &&
         a.content_framecount == b.content_framecount &&
         a.content_framerate == b.content_framerate &&
         a.content_duration == b.content_duration &&
         a.viewport_padding == b.viewport_padding &&
         a.is_sov_fixed_size == b.is_sov_fixed_size &&
         a.sov_fixed_size == b.sov_fixed_size &&
         a.is_block_fixed_size == b.is_block_fixed_size &&
         a.block_fixed_size == b.block_fixed_size &&
         a.is_tu_count_fixed == b.is_tu_count_fixed &&
         a.fixed_tu_count == b.fixed_tu_count &&
         a.numcode_byte_count == b.numcode_byte_count &&
         a.has_metadata == b.has_metadata &&
         a.encoded_blocks == b.encoded_blocks && a.icu_count == b.icu_count &&
         a.ldpc_coder_type == b.ldpc_coder_type &&
         a.ldpc_bp_ite == b.ldpc_bp_ite;
}

inline bool operator!=(const ContentParameterSet &a, const ContentParameterSet &b)
{
  return !(a == b);
}
} // namespace bitstream
} // namespace icov
