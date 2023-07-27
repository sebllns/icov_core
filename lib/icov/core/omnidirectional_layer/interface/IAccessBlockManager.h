/****************************************************************************/
/** @addtogroup Omnidirectional
 *  Omnidirectional is the adaptation of the coder into 360 context.
 *  @ingroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @file IAccessBlockManager.h
 *  @brief IAccessBlockManager is an interface for managing access blocks.
 *  @ingroup Omnidirectional
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
 * <b>For using ICOV under terms that will not comply with the GNU
 * AGPL, please contact Inria about acquiring an ICOV Professional
 * Edition License.</b>
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
#include "omnidirectional_layer/interface/IPicture.h"
#include "omnidirectional_layer/interface/IViewportManager.h"
#include <string>

namespace icov {
namespace omnidirectional_layer {

/**
 * @brief IAccessBlockManager is an interface for managing access blocks.
 *
 * The access block is defined as the smallest unit of the video that can be
 * accessed and decoded independently of the others.
 */
class IAccessBlockManager {
protected:
  BlockIteratorVector m_blocks; /**< Vector of access block iterators. */
  BlockVector::const_iterator
      m_invalidBlock; /**< Iterator pointing to an invalid block. */

public:
  //  IAccessBlockManager(const IPicture &pic)
  //      : m_current(pic.invalidBlock()), m_invalidBlock(pic.invalidBlock()) {}

  /**
   * @brief Virtual destructor for IAccessBlockManager.
   */
  virtual ~IAccessBlockManager() = default;

  /**
   * @brief Builds the access blocks based on the given picture and viewport
   * manager.
   *
   * This method is responsible for constructing the access blocks using the
   * provided picture and viewport manager.
   *
   * @param pic The picture to build the access blocks from.
   * @param vpm The viewport manager to use for building the access blocks.
   */
  virtual void buildAccessBlocks(IPicture &pic, IViewportManager &vpm) = 0;

  /**
   * @brief Returns the vector of access block iterators.
   *
   * @return The vector of access block iterators.
   */
  const BlockIteratorVector &blocks() const { return m_blocks; }

  /**
   * @brief Finds the access block using the given viewport manager.
   *
   * This method searches for the first visible access block using the provided
   * viewport manager.
   *
   * @param vpm The viewport manager used to determine block visibility.
   * @return An iterator pointing to the found access block, or m_invalidBlock
   * if no visible block is found.
   */
  virtual BlockVector::const_iterator
  findAccessBlock(const IViewportManager &vpm) const {
    for (const auto &b : m_blocks) {
      if (!b->hidden() && vpm.isBlockVisible(*b))
        return b;
    }
    return m_invalidBlock;
  }
};
} // namespace omnidirectional_layer
} // namespace icov
