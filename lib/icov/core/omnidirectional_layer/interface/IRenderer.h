/****************************************************************************/
/** @addtogroup Omnidirectional
 *  Omnidirectional is the adaptation of the coder into 360 context.
 *  @ingroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @file renderer.h
 *  @brief An abstract ICOV renderer for decoder side.
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

#include "icov_request.h"
namespace icov
{
namespace omnidirectional_layer {

/**

    @class IRenderer
    @brief An abstract renderer class for the decoder side.
    This class serves as a base class for other renderer classes.
    It contains a pointer to an IRequest object, which represents a request for
   a video frame. It also has a pointer to an array of unsigned char pixels,
   which represent the video frame that will be rendered.
    */
class IRenderer {
protected:
  request_t m_request; ///< A pointer to the IRequest object representing a
                       ///< request for a video frame.
  unsigned char
      *m_pixels; ///< A pointer to the array of unsigned char pixels
                 ///< representing the video frame that will be rendered.

  int m_width;  ///< An integer representing the width of the video frame.
  int m_height; ///< An integer representing the height of the video frame.

public:
  /** @brief IRenderer constructor
   *  Initializes the member variables to default values.
   */
  IRenderer() : m_pixels(nullptr), m_width(0), m_height(0)
  {
  }

  /** @brief IRenderer destructor
   *  Deallocates the memory allocated for the IRequest object and the pixels
   * array.
   */
  virtual ~IRenderer() {
    if (m_pixels != nullptr)
        delete m_pixels;

    m_pixels = nullptr;
  }

  /** @brief Renders the video frame on the screen.
   * Takes an array of pixels, as well as the width and height of the frame,
   * and renders the pixels on the screen.
      @param pixels An array of unsigned char pixels representing the video
   frame to be rendered.
      @param width An integer representing the width of the video frame.
      @param height An integer representing the height of the video frame.
      */
  virtual void drawPixels(unsigned char *pixels, int width, int height) = 0;

  /** @brief Sets the IRequest object for the renderer.
      @param r A pointer to the IRequest object representing a request for a
     video frame.
      */
  void setRequest(const request_t &r)
  {
    m_request = r;
  }

  /** @brief Returns a reference to the IRequest object.
      @return A const reference to the IRequest object representing a request
     for a video frame.
      */
  const request_t &getRequest() const
  {
    return m_request;
  }

  /** @brief Returns a const pointer to the array of unsigned char pixels
     representing the video frame that will be rendered. Returns a constant
     pointer to the buffer that stores the pixel data.
      @return A constant pointer to the pixel buffer.
      */
  const unsigned char *getPixels() const { return m_pixels; }

  /** @brief Get the width of the pixel buffer.
    Returns the width of the pixel buffer.
    @return The width of the pixel buffer.
    */
  int getWidth() const { return m_width; }

  /** @brief Get the height of the pixel buffer.
    Returns the height of the pixel buffer.
    @return The height of the pixel buffer.
    */
  int getHeight() const { return m_height; }
};

} // namespace omnidirectional_layer
} // namespace icov
