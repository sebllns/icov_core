/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup ICOV
 */

/** @file Block.h
 *  @brief Block conveying Viewport Units (VPU) for VCL data
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
#include "omnidirectional_layer/geometry.h"

class IPicture;

/** @class Block
 *  @brief Block
 *
 *  A Block is the smallest unit of a EquiRectPicture or Viewport and is
 * conveying the pixels to be decoded and the pixels to be displayed.
 */
class Block {

  friend class IPicture;

public:
  enum NeighborDirection {
    SELF = 0,
    RIGHT = 1,
    LEFT,
    UP,
    DOWN,
    COUNT,
  };

protected:
  IPicture *m_parent;
  float m_u;
  float m_v;
  float m_w;
  float m_h;
  bool m_isAccessBlock;
  size_t uid[NeighborDirection::COUNT];

  // center, topleft, topright, bottomright, bottomleft
  point3d cartesian_coords[5];

  Block(size_t id, IPicture *parent, float pu, float pv, float pw, float ph);

public:
  ~Block();

  const IPicture *parent() const;

  void setId(NeighborDirection index, size_t id);
  size_t getId(NeighborDirection index) const;
  const point3d &getCartesianCoord(uint16_t i) const;

  float u() const;
  float v() const;
  float w() const;
  float h() const;

  float center_u() const;
  float center_v() const;

  bool isAccessBlock() const;

  //  bool isVisible(const viewport_geometry &vpg) const;
  void setIsAccessBlock(bool newIsAccessBlock);
};

using sp_Block = std::shared_ptr<Block>;
using BlockVector = std::vector<Block>;
using BlockIteratorVector = std::vector<BlockVector::const_iterator>;
