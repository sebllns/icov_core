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

#include "omnidirectional_layer/geometry.h"
#include <memory>
#include <vector>
namespace icov {
namespace omnidirectional_layer {

class IPicture;

/**
 * @class Block
 * @brief The smallest unit of an EquiRectPicture or Viewport conveying pixels
 * to be decoded and displayed
 *
 * The Block class represents the smallest unit of an EquiRectPicture or
 * Viewport that conveys the pixels to be decoded and the pixels to be
 * displayed. It provides methods to set and get the IDs of neighboring blocks
 * in different directions and to retrieve the cartesian coordinates of its
 * center and four corners.
 */
class Block {
  friend class IPicture;

public:
  /**
   * The type used to store block IDs.
   */
  typedef unsigned long id_t;

  /**
   * Enumeration of neighboring directions.
   */
  enum NeighborDirection {
    SELF = 0,  /**< The block itself */
    RIGHT = 1, /**< The block to the right */
    LEFT,      /**< The block to the left */
    UP,        /**< The block above */
    DOWN,      /**< The block below */
    COUNT      /**< The total number of directions */
  };

  static std::string neighborDirectionStr(const NeighborDirection &dir) {
    switch (dir) {
    case SELF:
      return "SELF";
    case RIGHT:
      return "RIGHT";
    case LEFT:
      return "LEFT";
    case UP:
      return "UP";
    case DOWN:
      return "DOWN";
    }
    return "UNKNOWN";
  }

protected:
  IPicture *m_parent; /**< The parent picture object */
  float m_u; /**< The normalized horizontal coordinate of the top-left corner of
                the block */
  float m_v; /**< The normalized vertical coordinate of the top-left corner of
                the block */
  float m_w; /**< The normalized width of the block */
  float m_h; /**< The normalized height of the block */
  bool m_isAccessBlock; /**< Indicates whether the block is an access block or
                           not */
  Block::id_t uid[NeighborDirection::COUNT]; /**< IDs of neighboring blocks in
                                                different directions */

  point3d cartesian_coords[5]; /**< Cartesian coordinates of the block's center
                                  and four corners (center, topleft, topright,
                                  bottomright, bottomleft) */

  /**
   * Constructs a Block object with the given ID, parent picture, and
   * dimensions.
   *
   * @param id The ID of the block
   * @param parent The parent picture object
   * @param pu The horizontal coordinate of the top-left corner of the block
   * @param pv The vertical coordinate of the top-left corner of the block
   * @param pw The width of the block
   * @param ph The height of the block
   */
  Block(Block::id_t id, IPicture *parent, float pu, float pv, float pw,
        float ph);

public:
  /**
   * Destroys the Block object.
   */
  ~Block();

  /**
   * Returns a const pointer to the parent picture object.
   *
   * @return The parent picture object
   */
  const IPicture *parent() const;

  /**
   * Sets the ID of the neighboring block in the given direction.
   *
   * @param index The direction of the neighboring block
   * @param id The ID of the neighboring block
   */
  void setId(NeighborDirection index, Block::id_t id);

  /**
   * Returns the ID of the neighboring block in the given direction.
   *
   * @param index The direction of the neighboring block
   * @return The ID of the neighboring block
   */
  Block::id_t getId(NeighborDirection index) const;

  /**
   * Returns the ID of the block itself.
   *
   * @return The ID of the block itself
   */
  Block::id_t getIdSelf() const;

  /**
   * Returns the cartesian coordinates of the given corner.
   *
   * @param i The index of the corner (0 = center, 1 = top-left, 2 = top-right,
   * 3 = bottom-right, 4 = bottom-left)
   * @return const point3d& the cartesian coordinate at the specified index
   */
  const point3d &getCartesianCoord(uint16_t i) const;

  /**
   * @brief Gets the normalized horizontal coordinate of the top-left corner of
   * the block
   *
   * @return float the u coordinate of the block
   */
  float u() const;

  /**
   * @brief Gets the normalized vertical coordinate of the top-left corner of
   * the block
   *
   * @return float the v coordinate of the block
   */
  float v() const;

  /**
   * @brief Gets the normalized width of the block
   *
   * @return float the width of the block
   */
  float w() const;

  /**
   * @brief Gets the normalized height of the block
   *
   * @return float the height of the block
   */
  float h() const;

  /**
   * @brief Gets the center u coordinate of the block
   *
   * @return float the center u coordinate of the block
   */
  float center_u() const;

  /**
   * @brief Gets the center v coordinate of the block
   *
   * @return float the center v coordinate of the block
   */
  float center_v() const;

  /**
   * @brief Checks if the Block is an access block.
   *
   * @return True if the Block is an access block, false otherwise.
   */
  bool isAccessBlock() const;

  /**
   * @brief Sets whether the Block is an access block or not.
   *
   * @param newIsAccessBlock The new value to set for the access block flag.
   */
  void setIsAccessBlock(bool newIsAccessBlock);
};

/**
 * @brief Alias for a shared pointer to a Block.
 */
using sp_Block = std::shared_ptr<Block>;

/**
 * @brief Alias for a vector of Blocks.
 */
using BlockVector = std::vector<Block>;

/**
 * @brief Alias for a vector of constant iterators to Blocks.
 */
using BlockIteratorVector = std::vector<BlockVector::const_iterator>;

} // namespace omnidirectional_layer
} // namespace icov
