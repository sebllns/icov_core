/****************************************************************************/
/** @addtogroup icov-core
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup icov-core
 */

/** @file SymboleMode.h
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
#include "ldpc_codec_layer/IcovCoder.h"
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

///
/// \brief The SymboleMode class runs the tests sequences from a dataset of
/// blocks and SI values
///
class SymboleMode {
public:
  ///
  /// \brief AddProgramOptions add options specific to this mode
  /// \param desc
  ///
  static void AddProgramOptions(po::options_description &desc);

  ///
  /// \brief initialisation of this mode with the parsed options
  /// \param args
  ///
  SymboleMode(const po::variables_map &args);
  ~SymboleMode() = default;

  ///
  /// \brief run this mode
  ///
  void run();

private:
  ///
  /// \brief write_results writes a file containing measured rates from ICOV and
  /// the ground truth to compare
  ///
  /// \param sequence_name is the processed file
  /// \param numcodes are the rates measured
  ///
  void write_results(const std::string &sequence_name,
                     const std::vector<SINumcodeIdx> &numcodes);

  ///
  /// \brief process_input_list reads a text file that contains the list of
  /// files to be tested i.e. the dataset
  /// \param file is dataset: list of test data files separated by newline
  ///
  void process_input_list(const std::string &file);

  ///
  /// \brief process_input_file process one file frome the dataset
  /// \param file
  ///
  void process_input_file(const std::string &file);

  std::unique_ptr<AffInteractiveCoder> m_coder;

  fs::path m_dataset_dir;
  fs::path m_references_dir;
  fs::path m_result_dir;
  std::vector<float> q_values;
  std::ofstream measure_file;
  int max_si_count;

  bool enable_q_quantization;
  bool enable_z_quantization;

  static const std::string dataset_dir_id;
  static const std::string result_dir_id;
  static const std::string reference_dir_id;
  static const std::string max_si_count_id;
  static const std::string enable_q_quantization_id;
  static const std::string enable_z_quantization_id;
};
