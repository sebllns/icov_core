/****************************************************************************/
/** @addtogroup CLI
 *  Command Line Interface
 *  @ingroup ICOV
 */

/** @file CheckMode.h
 *  @brief BRIEF_DESCRIPTION
 *  DESCRIPTION
 *  @ingroup CLI
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

#include "FastArray.h"
#include "ldpc_codec_layer/factory/aff3ct_coder_types.h"
#include <boost/program_options.hpp>

namespace po = boost::program_options;

class CheckMode {
public:
  CheckMode(const po::variables_map &args, const std::string &data_path);
  ~CheckMode() = default;

  void run();

  static void AddProgramOptions(po::options_description &desc);

  std::string name() const { return "check"; }

private:
  int encode();
  void fill_src();
  void fill_pcrossover();
  bool check_decoding();
  void write_results();
  icov::FastMatrix<icov::bit> getHighLevelMatrix();
  icov::FastMatrix<icov::bit> getHighLevelMatrixImt164();
  icov::FastMatrix<uint16_t> getMappingVector(int m);
  int getNextLevel(int current);
  int getMinLevel();

  void compare_matrix(const icov::ldpc_codec_layer::AffMatrix &m,
                      const icov::FastMatrix<icov::bit> &gt);

  std::unique_ptr<icov::ldpc_codec_layer::AffInteractiveCoder> m_coder;
  icov::Syndrome m_stored_syndrome;
  std::string datapath;
};
