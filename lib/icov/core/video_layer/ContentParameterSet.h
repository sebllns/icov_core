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
#include "common.h"

#include <boost/serialization/access.hpp>

/** @class ContentParameterSet
 *  @brief class ContentParameterSet
 *
 *  Conveys the video parameters part of bitstream.
 */
struct ContentParameterSet {
  std::string icov_version;
  int content_type;
  int content_width;
  int content_height;
  int content_chroma_format;
  int content_framecount;
  float content_framerate;
  float content_duration;

  bool is_sov_fixed_size;
  int sov_fixed_size;

  bool is_block_fixed_size;
  int block_fixed_size;

  bool has_metadata;

  friend class boost::serialization::access;
  // When the class Archive corresponds to an output archive, the
  // & operator is defined similar to <<.  Likewise, when the class Archive
  // is a type of input archive the & operator is defined similar to >>.
  template <class Archive>
  void serialize(Archive &ar, const unsigned int version) {
    ar &icov_version;
    ar &content_type;
    ar &content_width;
    ar &content_height;
    ar &content_chroma_format;
    ar &content_framecount;
    ar &content_framerate;
    ar &content_duration;
    ar &is_sov_fixed_size;
    if (is_sov_fixed_size)
      ar &sov_fixed_size;
    ar &is_block_fixed_size;
    if (is_block_fixed_size)
      ar &block_fixed_size;
    ar &has_metadata;
  }
};
