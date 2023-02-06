/****************************************************************************/
/** @addtogroup Omnidirectional
 *  Omnidirectional is the adaptation of the coder into 360 context.
 *  @ingroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @file Viewport.h
 *  @brief Viewport conveying Viewport Units (VPU) for VCL data
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
#include "common.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/DecodingBlock.h"

using MinMax = std::pair<int16_t, int16_t>;
using BlockOrder = std::vector<int16_t>;
using RowMinMax = std::vector<MinMax>;

/** @class Viewport
 *  @brief Viewport
 *
 *  A Viewport is the smallest unit of a EquiRectPicture or Viewport and is
 * conveying the pixels to be decoded and the pixels to be displayed.
 */
class Viewport {
private:
public:
  std::vector<std::shared_ptr<DecodingBlock>> icus;
  BlockOrder order;
  RowMinMax rows;

  int16_t access_block_index;

  float center_x;
  float center_y;
  float fov_x;
  float fov_y;

  float u_min, u_max, v_min, v_max;

  Viewport(float pcenter_x, float pcenter_y, float fov_x, float fov_y);
  void computeOrdering();
  void computeRows();
  int16_t rowLength(int16_t row);
  void getBlockCoords(const Block &b, int16_t &o_row, int16_t &o_offset);
  int16_t getBlockIndex(int16_t row, int16_t offset);
  void findNextFreeBlock(int16_t &row, int16_t &offset, bool down);
  void normalizeUV(float &u, float &v);
};
