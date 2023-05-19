/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup ICOV
 */

/** @file LdpcCoderParameters.hpp
 *  @brief struct LdpcCoderParameters
 *  @ingroup ldpc_codec_layer
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

#include "common.h"

namespace icov {
struct LdpcCoderParameters {
  std::string type;                    ///< Type of LDPC code.
  std::string database;                ///< Path to the database.
  int codeLength;                      ///< Length of the code.
  int beliefPropagationIterations_new; ///< Number of iterations for belief
                                       ///< propagation.
  std::string
      numcodeFile; ///< Relative path from database to the numcodes file.

  LdpcCoderParameters() = default;

  /**
   * @brief Gets the numcodes vector for a given length of the code.
   * @param n Length of the code.
   * @param min_level Minimum level for numcodes.
   * @param max_level Maximum level for numcodes.
   * @return std::vector<int> Numcodes vector.
   */
  inline std::vector<int> getNumcodes(int n, int min_level,
                                      int max_level) const {
    std::string path = icov::path::combine(
        icov::path::get_data_folder(database, n, false), numcodeFile);
    std::vector<int> numcodes_v;

    int min_n = std::ceil(n / max_level);
    int max_n = std::floor(n / min_level);

    if (icov::path::file_exists(path)) {
      icov::io::read_vector(path, numcodes_v);
      icov::assert::check(numcodes_v.front() >= min_n,
                          "First numcode must be greater than " +
                              std::to_string(min_n));
      icov::assert::check(numcodes_v.back() <= max_n,
                          "Doping numcode must be less than " +
                              std::to_string(max_n));
    } else {
      ICOV_WARNING << "Numcodes file not found at " << path;
      ICOV_WARNING << "Creating default numcodes vector.";

      int max_doping_n = std::floor(1.5 * n);
      numcodes_v.resize(max_doping_n - min_n + 1);
      std::iota(numcodes_v.begin(), numcodes_v.end(), min_n);
      numcodes_v.push_back(n / 2);
    }
    return numcodes_v;
  }

  /**
   * @brief Clone the object of LdpcCoderParameters.
   * @return std::unique_ptr<LdpcCoderParameters> The cloned object of
   * LdpcCoderParameters.
   */
  virtual std::unique_ptr<LdpcCoderParameters> clone() const = 0;
  virtual ~LdpcCoderParameters() = default;
};
} // namespace icov
