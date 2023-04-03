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
#include "RequestManager.h"
#include "implementation/EquiRectPicture.h"
#include "implementation/IcovAccessBlockManager.h"
#include "implementation/ViewportManagerCenterPadded.h"
#include "interface/IPicture.h"
#include <memory>
#include <omnidirectional_layer/interface/IAccessBlockManager.h>
#include <omnidirectional_layer/interface/IPicture.h>
#include <omnidirectional_layer/interface/IViewportManager.h>

namespace icov {
namespace factory {
struct Visualization360 {
  std::shared_ptr<EquiRectPicture> Picture;
  std::shared_ptr<ViewportManagerCenterPadded> ViewportManager;
  std::shared_ptr<IcovAccessBlockManager> AccessBlockManager;

  struct params {
    uint Width, Height;
    float ViewportPadding;
  };

  explicit Visualization360(params p)
      : Picture(std::make_shared<EquiRectPicture>(p.Width, p.Height)),
        ViewportManager(
            std::make_shared<ViewportManagerCenterPadded>(p.ViewportPadding)),
        AccessBlockManager(std::make_shared<IcovAccessBlockManager>()) {}

  void init(ushort icu_width) {
    Picture->buildConstantSizeBlocks(icu_width, icu_width);
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

  void init(params p) {
    Visualization = std::make_unique<icov::factory::Visualization360>(
        p.VisualizationParams);
    Visualization->init(p.IcuSize);
    Request = std::make_unique<RequestManager>();
    Request->setPicture(Visualization->Picture);
    Request->setViewport(Visualization->ViewportManager);
    Request->setAccessBlocks(Visualization->AccessBlockManager);
  }
};

} // namespace factory
} // namespace icov
