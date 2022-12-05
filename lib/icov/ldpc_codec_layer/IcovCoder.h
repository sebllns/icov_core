/****************************************************************************/
/** @addtogroup icov-core
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup icov-core
 */

/** @file helpers.hpp
 *  @brief BRIEF_DESCRIPTION
 *  DESCRIPTION
 *  @ingroup common
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.0.1
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

#include "common.h"

#include "InteractiveCoder.h"
#include "LLRProvider.h"
#include "aff3ct/Aff3ctDecoder.h"
#include "helpers.h"
#include "icov_program_options.h"

//#include "implementation/imt/IMTSyndrome.h"
//#include "implementation/imt/IMTMatrixProvider.h"

//#include "implementation/imt_164/IMT164Syndrome.h"
//#include "implementation/imt_164/IMT164MatrixProvider.h"

//#include "implementation/varodayan/VarodayanSyndrome.hpp"
//#include "implementation/varodayan/VarodayanMatrixProvider.hpp"

using AffMatrix = aff3ct::tools::Sparse_matrix;
using AffInteractiveCoder = icov::InteractiveCoder<AffMatrix>;

#include <boost/program_options.hpp>
namespace po = boost::program_options;

using namespace icov;

namespace icov {
struct CoderParameters {
  std::string type;
  std::string database;
  std::string numcode_file;
  int n;
  int bp_ite;

  CoderParameters() = default;

  static std::string parseType(const po::variables_map &args) {
    return args[OPT_CODER_TYPE].as<std::string>();
  }

  static std::string parseDatabase(const po::variables_map &args) {
    return args[OPT_DATA_PATH].as<std::string>();
  }

  static int parseCodeLength(const po::variables_map &args) {
    return args[OPT_CODE_LENGTH].as<int>();
  }

  static int parseBpIte(const po::variables_map &args) {
    return args[OPT_BP_ITE].as<int>();
  }

  static std::string parseNumcodeFile(const po::variables_map &args) {
    return args[OPT_NUMCODES].as<std::string>();
  }

  CoderParameters(const po::variables_map &args,
                  const std::string &intented_type) {
    type = parseType(args);
    database = parseDatabase(args);
    n = parseCodeLength(args);
    bp_ite = parseBpIte(args);
    numcode_file = parseNumcodeFile(args);

    if (this->type != intented_type) {
      throw std::invalid_argument("Given type was \'" + type +
                                  "\' but intended type was \'" +
                                  intented_type + "\'\n");
    }
  }

  inline std::vector<int> getNumcodes(int n, int min_level,
                                      int max_level) const {
    std::string path = icov::helpers::path::combine(
        icov::helpers::path::get_data_folder(database, n, false), numcode_file);
    std::vector<int> numcodes_v;

    int min_n = std::ceil(n / max_level);
    int max_n = std::floor(n / min_level);

    if (icov::helpers::path::file_exists(path)) {
      icov::helpers::io::read_vector(path, numcodes_v);
      icov::helpers::assert::check(numcodes_v.front() >= min_n,
                                   "First numcode must be greater than " +
                                       std::to_string(min_n));
      icov::helpers::assert::check(numcodes_v.back() <= max_n,
                                   "Doping numcode must be less than " +
                                       std::to_string(max_n));
    } else {
      ICOV_LOG(warning) << "Numcodes file not found at " << path;
      ICOV_LOG(warning) << "Creating default numcodes vector.";

      int max_doping_n = std::floor(1.5 * n);
      numcodes_v.resize(max_doping_n - min_n + 1);
      std::iota(numcodes_v.begin(), numcodes_v.end(), min_n);
      numcodes_v.push_back(n / 2);
    }
    return numcodes_v;
  }

  virtual ~CoderParameters() = default;
};
} // namespace icov
