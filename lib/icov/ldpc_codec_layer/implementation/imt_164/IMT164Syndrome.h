/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup ICOV
 */

/** @file IMT164Syndrome.h
 *  @brief Manage syndrome from IMT matrices until rate 1/64
 *  DESCRIPTION
 *  @ingroup ldpc_codec_layer
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.5.0
 *  @date 2023
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

#include "implementation/imt_164/IMT164MatrixProvider.h"
#include "implementation/imt_164/IMT164SyndromeTools.h"
#include "ldpc_codec_layer/aff3ct/Aff3ctDecoder.h"
#include "ldpc_codec_layer/interface/ISyndrome.h"
#include <memory>

namespace icov {
namespace ldpc_codec_layer {

using index_t = uint16_t;

///
/// \brief The IMT164SyndromeManager class is manager for the syndrome given by
/// Hm * X where Hm is decoding matrix for rate m/n and X is the source code
/// vector of length n
///
class IMT164SyndromeManager : public ISyndromeManager {

public:
  // n = code length
  // min encoded syndrome length is n/max_level
  // max encoded syndrome length is n/min_level
  IMT164SyndromeManager(
      const IMT164SyndromeTools &tools,
      const std::shared_ptr<const IMT164MatrixProvider> &matrix_pvdr);
  virtual ~IMT164SyndromeManager();

  virtual void init_accumulation(const BitVec &source_x) override;

  virtual void compute_stored(Syndrome &o_stored_syndrome, int numcode,
                              LLRProvider &llr_provider,
                              const BitVec &src) override;

  virtual void compute_deacc(Syndrome &o_syndrome, const Syndrome &acc_syndrome,
                             int numcode, LLRProvider &llr_provider) override;

  virtual int get_m(int numcode) const override;

  std::string get_type() const override;

  //  void reset_doping() override;

  int get_m_max() const;

  int get_m_min() const;

  int get_numcode(int m) const;

  void compute_doping_order(LLRProvider &llr_provider, Syndrome &syndrome);

private:
  std::shared_ptr<const IMT164MatrixProvider> m_matrixPvdr;
  IMT164SyndromeTools m_tools;
  icov::bit *accumulated_syndrome;
  // Doping syndrome is cached
  Syndrome doping_synd;
};
} // namespace ldpc_codec_layer
} // namespace icov
