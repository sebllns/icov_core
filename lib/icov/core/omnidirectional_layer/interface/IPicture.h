/****************************************************************************/
/** @addtogroup Omnidirectional
 *  Omnidirectional is the adaptation of the coder into 360 context.
 *  @ingroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @file IPicture.h
 *  @brief IPicture conveying IPicture Units (VPU) for VCL data
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

#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/icov_geometry.h"
#include <boost/geometry/algorithms/transform.hpp>
#include <cstddef>
#include <vector>

namespace icov {
namespace omnidirectional_layer {

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

public:
  IPicture(size_t w, size_t h, float center_angle_x, float center_angle_y,
           float fov_x, float fov_y)
      : m_width(w), m_height(h), m_total_w(w), m_total_h(h),
        m_center_angle_x(center_angle_x), m_center_angle_y(center_angle_y),
        m_fov_x(fov_x), m_fov_y(fov_y) {}

  virtual ~IPicture() = default;

  const BlockVector &icus() const { return m_icus; }

  bool blockExists(Block::id_t id) const {
    return id > 0 && id <= m_icus.size();
  }

  BlockVector::iterator getById(Block::id_t id) {
    if (!blockExists(id))
      return m_icus.end();
    return m_icus.begin() + id - 1;
  }

  BlockVector::const_iterator getById(Block::id_t id) const {
    if (!blockExists(id))
      return m_icus.end();
    return m_icus.begin() + id - 1;
  }

  BlockVector::const_iterator getBlock(int i, int j) const {
    return getById(getId(i, j));
  }

  bool isValidBlock(BlockVector::const_iterator it) const {
    return it != invalidBlock();
  }

  void setAccessBlockFlag(Block::id_t id, bool value) {
    getById(id)->setIsAccessBlock(value);
  }

  void setVisibleBlocks(const std::vector<Block::id_t> &ids) {
    for (auto &b : m_icus)
      b.setHidden(true);

    for (const auto &i : ids)
      getById(i)->setHidden(false);
  }

  /** @addtogroup ICOV
   *  ICOV, open source Interactive Coder for Omnidirectional Videos.
   */

  /** @addtogroup common
   *  @ingroup ICOV
   */

  BlockVector::const_iterator invalidBlock() const { return m_icus.end(); }

  BlockVector::iterator addBlock(Block::id_t id, float pu, float pv, float pw,
                                 float ph) {
    m_icus.push_back(Block(id, this, pu, pv, pw, ph));
    return std::prev(m_icus.end());
  }

  size_t blockCount() const { return m_icus.size(); }

  virtual void getPixelCoords(float u, float v, int &i, int &j) const {
    i = this->row(u);
    j = this->col(v);
  }
  virtual int row(float v) const {
    return static_cast<int>(std::floor(v * static_cast<float>(m_total_h)));
  }
  virtual int col(float u) const {
    return static_cast<int>(std::floor(u * static_cast<float>(m_total_w)));
  }

  virtual float u(int i) const {
    return static_cast<float>(i) / static_cast<float>(m_total_w);
  }
  virtual float v(int j) const {
    return static_cast<float>(j) / static_cast<float>(m_total_h);
  }

  virtual size_t getId(int i, int j) const = 0;
  virtual float u(float angle_x) const = 0;
  virtual float v(float angle_y) const = 0;
  virtual float angleX(float u) const = 0;
  virtual float angleY(float v) const = 0;

  float minAngleX() const { return m_center_angle_x - 0.5f * m_fov_x; }
  float maxAngleX() const { return m_center_angle_x + 0.5f * m_fov_x; }
  float minAngleY() const { return m_center_angle_y - 0.5f * m_fov_y; }
  float maxAngleY() const { return m_center_angle_y + 0.5f * m_fov_y; }
  size_t width() const { return m_width; }
  size_t height() const { return m_height; }
  size_t total_w() const { return m_total_w; }
  size_t total_h() const { return m_total_h; }
  float center_angle_x() const { return m_center_angle_x; }
  float center_angle_y() const { return m_center_angle_y; }
  float fov_x() const { return m_fov_x; }
  float fov_y() const { return m_fov_y; }

  point3d getCartesianCoords(Block::id_t id) const {
    angle_radian p_spher(angleX(getById(id)->center_u()),
                         angleY(getById(id)->center_v()));
    point3d p_cart(0.0f, 0.0f, 0.0f);
    bg::transform(p_spher, p_cart);
    return p_cart;
  }
};

} // namespace omnidirectional_layer
} // namespace icov
