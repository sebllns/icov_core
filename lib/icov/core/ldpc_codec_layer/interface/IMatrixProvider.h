/****************************************************************************/
/** @addtogroup ICOV
 * ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup ldpc_codec_layer
 *  @ingroup icov
 */

/** @file IMatrixProvider.h
 *  @brief
 *  @ingroup interactive-coder
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 0.2.1
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

#include "common_types.h"
#include "tools/DebugTools.h"

namespace icov {
namespace ldpc_codec_layer {

template <class M> class IMatrixProvider {
public:
  IMatrixProvider() = default;
  virtual ~IMatrixProvider() = default;

  M &buildMatrix(int index) {
    ICOV_TRACE << __func__ << "(" << index << ") -> " << getNumcode(index);
    index = std::min(index, (int)m_matrices.size() - 1);
    _buildMatrix(getNumcode(index), getMatrix(index));
    return getMatrix(index);
  }

  const M &getMatrix(int index) const {
    return m_matrices[std::min(index, (int)m_matrices.size() - 1)];
  }

  M &getMatrix(int index) {
    return m_matrices[std::min(index, (int)m_matrices.size() - 1)];
  }

  void addNumcode(icov::index_t numcode, bool doping) {
    m_numcodes.push_back(numcode);
    if (!doping) {
      m_matrices.resize(m_numcodes.size());
      // std::cout << numcode << "->"  << m_matrices.size() << std::endl;
    }
  }

  int getMatrixCount() const { return (int)m_matrices.size(); }

  int getNumCodesCount() const { return (int)m_numcodes.size(); }

  icov::index_t getNumcode(int index) const { return m_numcodes[index]; }

  bool is_doping(int index) const { return index >= getMatrixCount(); }

protected:
  std::vector<icov::index_t> m_numcodes;
  std::vector<M> m_matrices;
  virtual void _buildMatrix(int numcode, M &matrix) = 0;
};
} // namespace ldpc_codec_layer
} // namespace icov
