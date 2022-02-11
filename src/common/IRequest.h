/****************************************************************************/
/** @addtogroup icov-core 
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */
 
/** @addtogroup common
 *  @ingroup icov-core
 */

/** @file request.h
 *  @brief Defines interface for Request object.
 *  @ingroup common
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

#ifndef IREQUEST_H
#define IREQUEST_H

#include "common.h"

/****************************************************************************/
/** @ingroup common
 *  @brief Defines the interface for any Request object sent to extractor.
 */
class IRequest
{
protected:
  float m_angleY;
  float m_angleX;
public:
  virtual void serialize() const  = 0;
  virtual void deserialize()  = 0;
  virtual void print()  = 0;

  void setAngleX(float value);
  void setAngleY(float value);

  float getAngleX() const;
  float getAngleY() const;

  virtual ~IRequest(){}
};

#endif // IREQUEST_H
