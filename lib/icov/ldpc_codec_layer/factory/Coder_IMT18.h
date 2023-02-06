/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup ICOV
 */

/** @file Coder_IMT18.hpp
 *  @brief BRIEF_DESCRIPTION
 *  DESCRIPTION
 *  @ingroup ldpc_codec_layer
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.0.3
 *  @date 2023
 *  @copyright Copyright (C) 2022 by Inria. All rights reserved.
 * The copyright in this software is being made available under
 * the terms of the GNU Affero Gener al Public License as published by
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

#include "IcovCoder.h"

#include <Tools/Algo/Matrix/Sparse_matrix/Sparse_matrix.hpp>
#include <memory>

using AffMatrix = aff3ct::tools::Sparse_matrix;

namespace icov {

class Coder_IMT18 {
  struct encode_parameters {
    icov::index_t start_code;
    icov::index_t stop_code;
    icov::index_t step;
  };

public:
  struct parameters : CoderParameters {
    parameters(const boost::program_options::variables_map &args)
        : CoderParameters(args, Coder_IMT18::id){

          };
    ~parameters() = default;
    std::unique_ptr<CoderParameters> clone() const override {
      return std::make_unique<parameters>(*this);
    }
  };

  static const std::string id;

  static void create(std::shared_ptr<LLRProvider> &llrProvider,
                     std::shared_ptr<IDecoder<AffMatrix>> &ldpca_dec,
                     std::shared_ptr<ISyndromeManager> &syndromeMgr,
                     std::shared_ptr<IMatrixProvider<AffMatrix>> &mb,
                     const parameters &p);
};

} // namespace icov
