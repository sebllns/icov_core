/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup ICOV
 */

/** @file icov_geometry.h
 *  @brief geometry conveying Viewport Units (VPU) for VCL data
 *  @ingroup common
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

#include <boost/geometry.hpp>
#include <boost/geometry/algorithms/transform.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/point_xyz.hpp>
#include <boost/qvm/all.hpp>

namespace icov {
namespace omnidirectional_layer {

namespace bg = boost::geometry;

typedef float coord_precision;

typedef bg::model::d2::point_xy<coord_precision, bg::cs::spherical<bg::degree>>
    angle_degree;
typedef bg::model::d2::point_xy<coord_precision, bg::cs::spherical<bg::radian>>
    angle_radian;
typedef bg::model::d3::point_xyz<coord_precision, bg::cs::cartesian> point3d;
typedef bg::model::d2::point_xy<coord_precision, bg::cs::cartesian> point2d;
typedef bg::model::box<point2d> box;

typedef boost::qvm::vec<float, 3> float3;

struct viewport_geometry {
  angle_radian pov;
  angle_radian fov;
  float focale;
  point2d resolution;
  point2d center;
  point2d roi_size;

public:
  angle_radian getPov() const { return pov; }
    void setPov(angle_radian newPov) { pov = newPov; }
};

bool is_point_visible(const point3d &p, const point2d &viewport_size);

point3d inverse_YPR(const point3d &p, float yaw, float pitch, float roll);
point3d rotate_YPR(const point3d &p, float yaw, float pitch, float roll);

inline angle_radian d2r(const angle_degree &d) {
  angle_radian r;
  bg::transform(d, r);
  return r;
}

inline angle_degree r2d(const angle_radian &r) {
  angle_degree d;
  bg::transform(r, d);
  return d;
}

inline bool is_point_visible(const point3d &p, const point2d &viewport_size) {
  // std::cout << viewport_size.x() << ", " << viewport_size.y() << std::endl;
  return p.x() > 0 && abs(p.y()) < p.x() * viewport_size.x() / 2 &&
         abs(p.z()) < p.x() * viewport_size.y() / 2;
}

inline point3d inverse_YPR(const point3d &p, float yaw, float pitch,
                           float roll) {
  float3 rot = boost::qvm::rot_mat_xyz<3, float>(-roll, -pitch, -yaw) *
               float3{p.x(), p.y(), p.z()};
  return point3d(boost::qvm::X(rot), boost::qvm::Y(rot), boost::qvm::Z(rot));
}

inline point3d rotate_YPR(const point3d &p, float yaw, float pitch, float roll) {
  float3 rot = boost::qvm::rot_mat_zyx<3, float>(yaw, pitch, roll) *
               float3{p.x(), p.y(), p.z()};
  return point3d(boost::qvm::X(rot), boost::qvm::Y(rot), boost::qvm::Z(rot));
}


} // namespace omnidirectional_layer
} // namespace icov
