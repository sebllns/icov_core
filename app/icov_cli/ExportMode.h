/****************************************************************************/
/** @addtogroup CLI
 *  Command Line Interface
 *  @ingroup ICOV
 */


/** @file ExportMode.h
 *  @brief BRIEF_DESCRIPTION
 *  DESCRIPTION
 *  @ingroup CLI
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.0.1
 *  @date 2022
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
#include "ldpc_codec_layer/IcovCoder.h"
#include <boost/program_options.hpp>
namespace po = boost::program_options;

class ExportMode {
public:
  static void AddProgramOptions(po::options_description &desc);
  ExportMode(const boost::program_options::variables_map &args, int code_length,
             const std::string &data_path);
  ~ExportMode() = default;
  void run();
  void export_matrices();
  void export_numcode();

  static const std::string rate_min_id;
  static const std::string rate_max_id;
  static const std::string rate_doping_id;
  static const std::string export_matrix_id;
  static const std::string export_numcode_id;

private:
  std::unique_ptr<AffInteractiveCoder> m_coder;

  bool export_matrix_enabled;
  int export_numcode_mode;
  float min_rate, max_rate, doping_rate;
  int n;
  po::variables_map m_options_map;
  std::string datapath;
  std::string numcodes_file;
};
