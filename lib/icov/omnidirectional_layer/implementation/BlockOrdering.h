/****************************************************************************/
/** @addtogroup Omnidirectional
 *  Omnidirectional is the adaptation of the coder into 360 context.
 *  @ingroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @file BlockOrdering.h
 *  @brief BlockOrdering Performs the loop on blocks starting from a viewport
 * request, until all blocks in the viewport are processed in the order defined
 * by the order heuristic
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
#include "omnidirectional_layer/interface/IBlockOrdering.h"

#include <stack>

namespace icov {
namespace omnidirectional_layer {

/** @class BlockOrdering
 *  @brief BlockOrdering class performs the loop on blocks starting from a
 *viewport request, until all blocks in the viewport are processed in the order
 *defined by the order heuristic
 */
class BlockOrdering : public IBlockOrdering {
protected:
  /** @brief The block_path_info struct puts in cache many useful informations
   * to order the blocks faster
   */
  struct block_path_info {
    bool visibility_tested;
    bool is_visible;
    bool is_processed;
    bool si_horizontal;
    bool si_vertical;

    block_path_info() { reset(); }

    void reset() {
      visibility_tested = false;
      is_visible = false;
      is_processed = false;
      si_horizontal = false;
      si_vertical = false;
    }
  };

  std::stack<size_t> m_cursor;
  std::vector<size_t> m_elements;
  std::vector<block_path_info> m_path_info;
  std::vector<block_path_info>::iterator getPathInfoById(size_t id);
  std::vector<block_path_info>::const_iterator getPathInfoById(size_t id) const;

  /**
   * @brief Find the first available neighbor of the current block.
   *
   * @param current The current block being processed i.e. the block to find an
   * available neighbor for.
   * @param vpm The viewport manager as an instance of the IViewportManager
   * interface.
   * @return Block::NeighborDirection The direction of the available neighbor
   * block, or Block::COUNT if none are available.
   */
  Block::NeighborDirection findAvailableNeighbor(const Block &current,
                                                 const IViewportManager &vpm);

public:
  BlockOrdering(size_t blocks_count);
  ~BlockOrdering() = default;

  //    template<typename Iterator> void
  //    reset(Iterator it, Iterator end)
  //    {
  //        for(; it != end; ++it) this->getById(*it)->reset();
  //    }

  // IBlockOrdering interface
public:
  /**
   * @brief Get the iterator for the next block to be processed after the
   * current block.
   *
   * @details Function getNextBlock, which searches for the
   * next block that should be processed. The function checks for the visibility
   * of each neighbor block and updates their visibility information. It starts
   * by getting the ID of the neighbor block based on the given direction. If
   * the neighbor block exists and has not been processed before, it gets its
   * path information by calling getPathInfoById(). If the visibility of the
   * neighbor block has not been tested before, the function appends its ID to
   * the m_elements vector and calls isBlockVisible() on vpm to update its
   * visibility information. After that, the function checks whether the
   * neighbor block is visible. If it is visible, the function sets the
   * corresponding si_horizontal or si_vertical flag in the path information of
   * the neighbor block, depending on the direction of the neighbor. If the
   * neighbor block is not visible, no flag is set.
   *
   * @note Note that the isBlockProcessed() function is used to check whether a
   * block has already been processed. If a block has been processed, its
   * visibility information has already been updated, and it does not need to be
   * processed again.
   *
   * @param current The current block being processed.
   * @param vpm The viewport manager as an instance of the IViewportManager
   * interface.
   * @return BlockVector::const_iterator A constant iterator to the next block
   * in the ordering, or an iterator to an invalid block if no such block is
   * found (means no more block to process).
   */
  BlockVector::const_iterator
  getNextBlock(const Block &current, const IViewportManager &vpm) override;
  void reset() override;
  bool isBlockProcessed(size_t id) const override;
  bool blockExists(size_t id) const;
};
} // namespace omnidirectional_layer
} // namespace icov
