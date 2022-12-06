/****************************************************************************/
/** @addtogroup Video
 *  Video contains the code related to video compression.
 *  @ingroup ICOV
 */

/** @file ICOV_BufGroups.h
 *  @brief ICOV_BufGroups
 *  @ingroup Video
 *  @author Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.0.0
 *  @date 2018, 2022
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
#include "CommonLib/Picture.h"
#include "ICOV_Storage.h"
#include <map>

namespace icov::video {

class BufGroups {

  using const_iterator = std::map<int, PelStorage>::const_iterator;
  using iterator = std::map<int, PelStorage>::iterator;

protected:
  BoolStorage _recoFlagBuf;

  std::map<int, PelStorage> _bufs;
  std::map<int, int> _links;
  //  UInt _margins[NUM_PIC_TYPES);

public:
  BufGroups();
  void destroy();
  ~BufGroups();

  void create(int pictureType, const ChromaFormat &chromaFormat,
              const Size &lumaSize, const unsigned maxCUSize,
              const unsigned margin, const unsigned alignment = 0,
              const bool scaleChromaMargin = true);
  void create(int pictureType, const UnitArea &unit);

  inline void create_link(int from, int to) { _links[from] = _links[to]; }

  inline const_iterator begin() const { return _bufs.begin(); }
  inline const_iterator end() const { return _bufs.end(); }

  inline iterator begin() { return _bufs.begin(); }
  inline iterator end() { return _bufs.end(); }

  PelStorage &get(int pictureType) { return _bufs.at(_links[pictureType]); }
  const PelStorage &get(int pictureType) const {
    return _bufs.at(_links.at(pictureType));
  }

  PelUnitBuf &getBuf(int pictureType) { return _bufs.at(_links[pictureType]); }
  const CPelUnitBuf getBuf(int pictureType) const {
    return _bufs.at(_links.at(pictureType));
  }
};
} // namespace icov::video
