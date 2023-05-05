/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup ICOV
 */

/** @file request.h
 *  @brief Defines interface for Request object.
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

namespace icov {
namespace omnidirectional_layer {
/****************************************************************************/
/**
 * @brief Interface definition for a Request object used in the ICOV system.
 *
 * This abstract class defines the interface for a Request object used in the
 * Interactive Coder for Omnidirectional Videos (ICOV) project. A Request object
 * represents a viewpoint of the viewer's navigation in a 360-degree video,
 * specified by its Y and X angles in the video frame.
 */
class IRequest {
protected:
  float m_angleY; /**< Y angle of the Request object, representing the vertical
                     rotation of the viewer's viewpoint in the video frame. */
  float
      m_angleX; /**< X angle of the Request object, representing the horizontal
                   rotation of the viewer's viewpoint in the video frame. */

public:
  /**
   * @brief Virtual destructor for the IRequest class.
   */
  virtual ~IRequest() {}

  /**
   * @brief Get the current Y angle of the Request object.
   * @return The current Y angle of the Request object, representing the
   * vertical rotation of the viewer's viewpoint in the video frame.
   */
  float angleY() const { return m_angleY; }

  /**
   * @brief Set the Y angle of the Request object to a new value.
   * @param newAngleY The new Y angle to set, representing the vertical rotation
   * of the viewer's viewpoint in the video frame.
   */
  void setAngleY(float newAngleY) { m_angleY = newAngleY; }

  /**
   * @brief Get the current X angle of the Request object.
   * @return The current X angle of the Request object, representing the
   * horizontal rotation of the viewer's viewpoint in the video frame.
   */
  float angleX() const { return m_angleX; }

  /**
   * @brief Set the X angle of the Request object to a new value.
   * @param newAngleX The new X angle to set, representing the horizontal
   * rotation of the viewer's viewpoint in the video frame.
   */
  void setAngleX(float newAngleX) { m_angleX = newAngleX; }
};

} // namespace omnidirectional_layer
} // namespace icov
