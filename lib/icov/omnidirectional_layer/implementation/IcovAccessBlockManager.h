/****************************************************************************/
/** @addtogroup icov-core
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup icov-core
 */

/** @file IcovAccessBlockManager.h
 *  @brief IcovAccessBlockManager conveying IcovAccessBlockManager Units (VPU) for VCL data
 *  @ingroup common
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.0.1
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
#include "omnidirectional_layer/interface/IAccessBlockManager.h"

/** @class IcovAccessBlockManager
 *  @brief IcovAccessBlockManager
 *
 *  A IcovAccessBlockManager is the smallest unit of a EquiRectPicture or IcovAccessBlockManager and is conveying the pixels to be decoded and the pixels to be displayed.
 *  TODO : RENAME WITHOUT ICOV
 */
class IcovAccessBlockManager : public IAccessBlockManager
{
protected:
    angle_radian m_angle_delta;
public:
    IcovAccessBlockManager() = default;
    IcovAccessBlockManager(angle_radian angle_delta);
    ~IcovAccessBlockManager() = default;

    // IAccessBlockManager interface
    virtual void buildAccessBlocks(const IPicture &pic, IViewportManager &vpm) override;
};
