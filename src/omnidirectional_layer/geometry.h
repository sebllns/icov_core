/****************************************************************************/
/** @addtogroup icov-core
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup icov-core
 */

/** @file geometry.h
 *  @brief geometry conveying Viewport Units (VPU) for VCL data
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
#pragma once

#include "common.h"
#include "boost/geometry.hpp"
#include <boost/geometry/geometries/point_xyz.hpp>
//#include <boost/qvm/quat_operations.hpp>
#include <boost/qvm/all.hpp>

namespace bg = boost::geometry;

typedef float coord_precision;
typedef bg::model::d2::point_xy<coord_precision, bg::cs::spherical<bg::degree>>     angle_degree;
typedef bg::model::d2::point_xy<coord_precision, bg::cs::spherical<bg::radian> >    angle_radian;
typedef bg::model::d3::point_xyz<coord_precision, bg::cs::cartesian >               point3d;
typedef bg::model::d2::point_xy<coord_precision, bg::cs::cartesian >                point2d;
typedef bg::model::box<point2d>                                                     box;

typedef boost::qvm::vec<float, 3> float3;


struct viewport_geometry
{
//    enum visibility_test_strategy_t
//    {
//        NONE = 0,
//        CENTER_WITH_PADDING,
//        CENTER_AND_CORNERS,
//    };

    angle_radian pov;
    angle_radian fov;
    float focale;
    point2d resolution;
    point2d center;
    point2d roi_size;
//    visibility_test_strategy_t visibility_test_strategy;

public:
    angle_radian getPov() const;
    void setPov(angle_radian newPov);
};

bool is_visible(const point3d &p, const point2d &viewport_size);

point3d inverse_YPR(const point3d &p, float yaw, float pitch, float roll);
point3d rotate_YPR(const point3d &p, float yaw, float pitch, float roll);
