/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup ICOV
 */

/** @file ViewportUnit.h
 *  @brief ViewportUnit (VPU) for VCL data
 *  @ingroup common
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

#include "InteractiveCodedUnit.h"
#include "NalUnit.h"
#include <fstream>

namespace icov {
namespace bitstream {

/** @class ViewportUnit
 *  @brief ViewportUnit
 *
 *  A ViewportUnit is a part of ICOV bitstream that is conveying a set of
 * Interactive Coded Units (ICUs) as Video Coding Layer data.
 */
class ViewportUnit {

public:
  ViewportUnit() = default;
  NalUnit ToNal();
  bool TryParse(NalUnit nal);
  void addIcu(InteractiveCodedUnit icu);

  int getIcuCount() { return m_icus.size(); }
  const std::vector<InteractiveCodedUnit> &getIcus() const { return m_icus; }

private:
  std::vector<InteractiveCodedUnit> m_icus;
};
} // namespace bitstream
} // namespace icov
