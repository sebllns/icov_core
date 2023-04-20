/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup ICOV
 */

/** @file LLRProvider.h
 *  @brief LLRProvider is a class used to compute LLR for LDPC coder
 *  @ingroup ldpc_codec_layer
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.0.3
 *  @date %YEAR%
 *  @copyright Copyright (C) %YEAR% by Inria. All rights reserved.
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

#include "BitSymbolProvider.h"

namespace icov {
namespace ldpc_codec_layer {
///
/// \brief The LLRProvider class computes LLR and manages Doping for LDPC rate
/// adaptive coder
///
class LLRProvider {
public:
  LLRProvider(int n);
  LLRProvider(const std::vector<LLRVal> &llr);

  static LLRVal limit_max();
  static LLRVal epsilon();

  // inline LLRVal &operator[](int i) { return m_llr[i]; }
  // inline LLRVal operator[](int i) const { return m_llr[i]; }
  inline const std::vector<LLRVal> &llr() const {
    return m_doping_ready ? m_llr_doping : m_llr;
  }

  inline const std::vector<size_t> &llr_order() const { return m_doping_order; }

  inline std::vector<size_t> &llr_order() { return m_doping_order; }

  // inline const std::vector<LLRVal> &llr_doping() const { return m_llr_doping;
  // }

  void initLLR(const std::vector<SIVal> &side_info, const BitVec &src_bits);
  void initLLR(float pCrossover, const BitVec &src);
  void initLLRUniform(float pCrossover, const BitVec &src);
  std::vector<icov::BitVal> initSrcRand(int n);

  void init_doping();
  void apply_doping(int index, BitVal x);
  int doping_order(int index) const;
  bool doping_ready() const;

  uint32_t index_min() const;

  void reset();

  void initLLR_s(const BitSymbolProvider &bsp,
                 const std::vector<SIVal> &side_info);
  void initLLR_b(int b, const BitSymbolProvider &bsp,
                 const std::vector<SIVal> &side_info,
                 const std::vector<int> &sign,
                 const std::vector<int> &abs_predec);

  bool no_coding(const BitVec &src) const;

protected:
  std::vector<LLRVal> m_llr;
  std::vector<LLRVal> m_llr_doping;
  std::vector<size_t> m_doping_order;
  bool m_doping_ready;

  bool m_floor_type;
};
} // namespace ldpc_codec_layer
} // namespace icov
