/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup ICOV
 */

/** @file IPicture.h
 *  @brief IPicture conveying IPicture Units (VPU) for VCL data
 *  @ingroup common
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

#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/geometry.h"

class RequestManager;

/** @class IPicture
 *  @brief IPicture
 *
 *  A IPicture is the smallest unit of a IPicture or IPicture and is conveying
 * the pixels to be decoded and the pixels to be displayed.
 */
class IPicture {
protected:
  size_t m_width;
  size_t m_height;
  size_t m_total_w;
  size_t m_total_h;
  float m_center_angle_x;
  float m_center_angle_y;
  float m_fov_x;
  float m_fov_y;

  BlockVector m_icus;

  RequestManager *m_requestManager;

public:
  IPicture(size_t w, size_t h, float center_angle_x, float center_angle_y,
           float fov_x, float fov_y);

  virtual ~IPicture() = default;

  const BlockVector &icus() const;

  bool blockExists(size_t id) const;

  BlockVector::iterator getById(size_t id);
  BlockVector::const_iterator getById(size_t id) const;

  BlockVector::const_iterator getBlock(int i, int j) const;

  bool isValidBlock(BlockVector::const_iterator it) const;

  void setAccessBlockFlag(size_t id);

  BlockVector::const_iterator invalidBlock() const;

  BlockVector::iterator addBlock(size_t id, float pu, float pv, float pw,
                                 float ph);

  virtual void getPixelCoords(float u, float v, int &i, int &j) const;
  virtual int row(float v) const;
  virtual int col(float u) const;

  virtual float u(int i) const;
  virtual float v(int j) const;

  virtual size_t getId(int i, int j) const = 0;
  virtual float u(float angle_x) const = 0;
  virtual float v(float angle_y) const = 0;
  virtual float angleX(float u) const = 0;
  virtual float angleY(float v) const = 0;

  float minAngleX() const;
  float maxAngleX() const;
  float minAngleY() const;
  float maxAngleY() const;
  size_t width() const;
  size_t height() const;
  size_t total_w() const;
  size_t total_h() const;
  float center_angle_x() const;
  float center_angle_y() const;
  float fov_x() const;
  float fov_y() const;

  point3d getCartesianCoords(size_t id) const;

  RequestManager *requestManager() const;
  void setRequestManager(RequestManager *newRequestManager);
  void releaseRequestManager();
};
