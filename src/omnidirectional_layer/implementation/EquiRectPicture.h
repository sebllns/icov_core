/****************************************************************************/
/** @addtogroup icov-core
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup icov-core
 */

/** @file EquiRectPicture.h
 *  @brief EquiRectPicture conveying EquiRectPicture Units (VPU) for VCL data
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

#include "omnidirectional_layer/interface/IPicture.h"


/** @class EquiRectPicture
 *  @brief EquiRectPicture
 *
 *  A EquiRectPicture is the smallest unit of a EquiRectPicture or EquiRectPicture and is conveying the pixels to be decoded and the pixels to be displayed.
 */
class EquiRectPicture : public IPicture
{
private:

    int block_rows;
    int block_cols;

    // IPicture interface
public:
    ~EquiRectPicture() = default;

    virtual size_t getId(int i, int j) const override;
    virtual float u(float angle_x) const override;
    virtual float v(float angle_y) const override;
    virtual float angleX(float u) const override;
    virtual float angleY(float v) const override;



    void buildConstantSizeBlocks(int pw, int ph);
    EquiRectPicture(int pw, int ph);


};
