/****************************************************************************/
/** @addtogroup icov-core 
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup decoder
 * Decoder package (client side).
 *  @ingroup icov-core
 */

/** @file renderer.h
 *  @brief An abstract ICOV renderer for decoder side.
 *  @ingroup decoder
 *  @author Sébastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 0.0.1
 *  @date 2021
 *  @copyright Copyright (C) 2021 by Inria. All rights reserved.
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

#ifndef IRENDERER_H
#define IRENDERER_H

#include "request.h"
#include "common.h"

class IRenderer
{
protected:
  IRequest *m_request;
  unsigned char *m_pixels;
  int m_width;
  int m_height;

public:
  IRenderer() : m_request(nullptr),
                m_pixels(nullptr),
                m_width(0),
                m_height(0) {}
  virtual ~IRenderer();
  virtual void drawPixels(unsigned char *pixels, int width, int height) = 0;
  void setRequest(IRequest *);
  const IRequest &getRequest() const;
  const unsigned char *getPixels() const;
  const int getWidth() const;
  const int getHeight() const;
};

#endif // IRENDERER_H
