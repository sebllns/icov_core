/****************************************************************************/
/** @addtogroup icov-core
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup icov-core
 */

/** @file Picture.h
 *  @brief Picture conveying Picture Units (VPU) for VCL data
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

#include "Block.h"
#include "Viewport.h"

/** @class Picture
 *  @brief Picture
 *
 *  A Picture is the smallest unit of a Picture or Picture and is conveying the pixels to be decoded and the pixels to be displayed.
 */
class Picture
{
private:

public:
    BlockVector icus;
    BlockVector access_blocks;
    int w;
    int h;
    int total_w;
    int total_h;
    int block_cols;
    int block_rows;
    float center_x;
    float center_y;
    float fov_x;
    float fov_y;
    Picture(int pw, int ph);
    void buildConstantSizeBlocks(int w, int h);
    shared_ptr<Viewport> getViewport(float center_x, float center_y, float fov_x, float fov_y);
    shared_ptr<Block> getBlock(int i, int j);
    void getBlockCoords(float u, float v, int &i, int &j);
    float u(float theta);
    float v(float phi);
};
