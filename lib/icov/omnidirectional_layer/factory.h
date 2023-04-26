/****************************************************************************/
/** @addtogroup Omnidirectional
 *  Omnidirectional is the adaptation of the coder into 360 context.
 *  @ingroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @file factory.h
 *  @brief factory Performs the loop on blocks starting from a viewport
 *request, until all blocks in the viewport are processed in the order defined
 *by the order heuristic
 *  @ingroup Omnidirectional
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
#include "implementation/BlockOrdering.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "omnidirectional_layer/RequestManager.h"
#include "omnidirectional_layer/geometry.h"
#include "omnidirectional_layer/implementation/EquiRectPicture.h"
#include "omnidirectional_layer/implementation/IcovAccessBlockManager.h"
#include "omnidirectional_layer/implementation/ViewportManagerCenterPadded.h"
#include "omnidirectional_layer/interface/IPicture.h"
#include "typedef.h"
#include <boost/geometry/algorithms/transform.hpp>
#include <math.h>
#include <memory>
#include <valarray>

// Fallback, in case M_PI doesn't exist.
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace icov {
namespace omnidirectional_layer {
namespace factory {

inline float x_y_ratio(const point2d &resolution) {
  return resolution.x() / resolution.y();
}

inline float y_x_ratio(const point2d &resolution) {
  return 1.0 / x_y_ratio(resolution);
}

inline angle_radian fov(float fov_x_degrees, float y_x_ratio) {
  float fovX_radian = fov_x_degrees * DEG2RAD;
  float fovY_radian = 2 * atan(tan(fovX_radian * 0.5) * y_x_ratio);
  return angle_radian(fovX_radian, fovY_radian);
}

inline viewport_geometry default_geometry(const angle_radian &fov,
                                          const point2d &screen_resolution) {
  viewport_geometry vpg;
  vpg.focale = 1 / (2.0f * tan(fov.x() * 0.5)); // focale
  vpg.center = point2d(0.5f, 0.5f);             // center normalised coords
  vpg.fov = fov;                                // field of view in radians
  vpg.resolution = screen_resolution;           // resolution in pixels

  // viewport geometry in cartesian coords
  vpg.roi_size =
      point2d(std::abs(2 * tan(fov.x())), std::abs(2 * tan(fov.y())));

  return vpg;
}

struct Visualization360 {
  std::shared_ptr<EquiRectPicture> Picture;
  std::shared_ptr<ViewportManagerCenterPadded> ViewportManager;
  std::shared_ptr<IcovAccessBlockManager> AccessBlockManager;

  ///
  /// \brief The params class
  ///
  struct params {
    uint Width, Height;
    float ViewportPadding;
  };

  explicit Visualization360(params p)
      : Picture(std::make_shared<EquiRectPicture>(p.Width, p.Height)),
        ViewportManager(
            std::make_shared<ViewportManagerCenterPadded>(p.ViewportPadding)),
        AccessBlockManager(std::make_shared<IcovAccessBlockManager>()) {}

  void init(ushort icu_width, const viewport_geometry &vpg) {
    Picture->buildConstantSizeBlocks(icu_width, icu_width);
    ViewportManager->viewportGeometry() = vpg;
    AccessBlockManager->buildAccessBlocks(*Picture, *ViewportManager);
  }
};

struct Navigation360 {
  std::unique_ptr<Visualization360> Visualization;
  std::unique_ptr<RequestManager> Request;

  struct params {
    Visualization360::params VisualizationParams;
    ushort IcuSize;
  };

  void init(params p, const viewport_geometry &vpg) {
    Visualization =
        std::make_unique<factory::Visualization360>(p.VisualizationParams);
    Visualization->init(p.IcuSize, vpg);
    Request = std::make_unique<RequestManager>();
    Request->setPicture(Visualization->Picture);
    Request->setViewport(Visualization->ViewportManager);
    Request->setAccessBlocks(Visualization->AccessBlockManager);
    Request->setBlockPrediction(std::make_shared<BlockPrediction>());
    Request->setOrdering(
        std::make_shared<BlockOrdering>(Visualization->Picture->blockCount()));
  }
};

} // namespace factory
} // namespace omnidirectional_layer
} // namespace icov
