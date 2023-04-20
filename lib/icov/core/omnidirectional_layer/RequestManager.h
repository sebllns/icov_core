/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup ICOV
 */

/** @file RequestManager.h
 *  @brief RequestManager conveying RequestManager Units (VPU) for VCL data
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

#include "Block.h"
#include <memory>

namespace icov {
namespace omnidirectional_layer {
class IBlockOrdering;
class IViewportManager;
class IBlockProcessor;
class IAccessBlockManager;
class IBlockPredictionManager;

/** @class RequestManager
 *  @brief RequestManager
 *
 *  A RequestManager is
 */
class RequestManager {

public:
private:
  std::shared_ptr<IAccessBlockManager> m_accessBlocks;
  std::shared_ptr<IBlockOrdering> m_ordering;
  std::shared_ptr<IViewportManager> m_viewport;
  std::shared_ptr<IPicture> m_picture;
  std::shared_ptr<IBlockProcessor> m_blockProcessor;
  std::shared_ptr<IBlockPredictionManager> m_blockPrediction;

public:
  RequestManager() = default;
  ~RequestManager() = default;

  void start();
  void update(float angle_x, float angle_y);

  const std::shared_ptr<IBlockOrdering> &ordering() const;
  void setOrdering(const std::shared_ptr<IBlockOrdering> &newOrdering);
  const std::shared_ptr<IViewportManager> &viewport() const;
  void setViewport(const std::shared_ptr<IViewportManager> &newViewport);
  const std::shared_ptr<IPicture> &picture() const;
  void setPicture(const std::shared_ptr<IPicture> &newPicture);
  const std::shared_ptr<IBlockProcessor> &blockProcessor() const;
  void
  setBlockProcessor(const std::shared_ptr<IBlockProcessor> &newBlockProcessor);
  const std::shared_ptr<IAccessBlockManager> &accessBlocks() const;
  void
  setAccessBlocks(const std::shared_ptr<IAccessBlockManager> &newAccessBlocks);
  const std::shared_ptr<IBlockPredictionManager> &blockPrediction() const;
  void setBlockPrediction(
      const std::shared_ptr<IBlockPredictionManager> &newBlockPrediction);
};
} // namespace omnidirectional_layer
} // namespace icov
