/****************************************************************************/
/** @addtogroup Omnidirectional
 *  Omnidirectional is the adaptation of the coder into 360 context.
 *  @ingroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @file IViewportManager.h
 *  @brief Interface for a viewport manager that provides information about the
 * visibility of a block within a viewport geometry.
 *  @ingroup Omnidirectional
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

#include "omnidirectional_layer/icov_geometry.h"

namespace icov {
namespace omnidirectional_layer {

class Block;

/**
 * @class IViewportManager
 * @brief Interface for a viewport manager that provides information about the
 * visibility of a block within a viewport geometry.
 */
class IViewportManager {
private:
  viewport_geometry m_viewportGeometry; /**< The viewport geometry */

public:
  virtual ~IViewportManager() = default;

  /**
   * @brief Determines if a block is visible within the viewport geometry.
   * @param b The block to check for visibility.
   * @return True if the block is visible, false otherwise.
   */
  virtual bool isBlockVisible(const Block &b) const = 0;

  /**
   * @brief Gets the const reference to the viewport geometry.
   * @return The const reference to the viewport geometry.
   */
  const viewport_geometry &viewportGeometry() const {
    return m_viewportGeometry;
  }

  /**
   * @brief Gets the reference to the viewport geometry.
   * @return The reference to the viewport geometry.
   */
  viewport_geometry &viewportGeometry() { return m_viewportGeometry; }
};
} // namespace omnidirectional_layer
} // namespace icov
