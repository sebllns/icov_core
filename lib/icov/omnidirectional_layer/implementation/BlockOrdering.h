/****************************************************************************/
/** @addtogroup Omnidirectional
 *  Omnidirectional is the adaptation of the coder into 360 context.
 *  @ingroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @file BlockOrdering.h
 *  @brief BlockOrdering Performs the loop on blocks starting from a viewport
 *request, until all blocks in the viewport are processed in the order defined
 *by the order heuristic
 *  @ingroup Omnidirectional
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
#include "omnidirectional_layer/interface/IBlockOrdering.h"

#include <stack>

/** @class BlockOrdering
 *  @brief BlockOrdering class performs the loop on blocks starting from a
 *viewport request, until all blocks in the viewport are processed in the order
 *defined by the order heuristic
 */
class BlockOrdering : public IBlockOrdering {
protected:
  ///
  /// \brief The block_path_info struct puts in cache many useful informations
  /// to order the blocks faster
  ///
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
  BlockVector::const_iterator
  getNextBlock(const Block &current, const IViewportManager &vpm) override;
  void reset() override;
  bool isBlockProcessed(size_t id) const override;
  bool blockExists(size_t id) const;
};
