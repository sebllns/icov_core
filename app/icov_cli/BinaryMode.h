/****************************************************************************/
/** @addtogroup icov-core
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup icov-core
 */

/** @file BinaryMode.h
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
#include <boost/program_options.hpp>
namespace po = boost::program_options;

class BinaryMode {
public:
  BinaryMode(const po::variables_map &args);
  ~BinaryMode() = default;

  void run();

  static void AddProgramOptions(po::options_description &desc);

private:
  int encode();
  void fill_src();
  void fill_pcrossover();
  bool check_decoding();
  void write_results();

  std::unique_ptr<AffInteractiveCoder> m_coder;
  BitVec m_src_bits;
  BitVec m_dec_bits;
  std::vector<double> m_pcrossover;
  Syndrome m_stored_syndrome;
  uint32_t m_numcodeIdx;
  uint32_t m_error_count;
  int m_nite;

  //  std::vector<LLRVal> llr;
  std::vector<double> time;
  std::vector<double> decoding_time;
  std::vector<double> numerrors;
  std::vector<double> rate;
};
