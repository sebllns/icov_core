/****************************************************************************/
/** @addtogroup CLI
 *  Command Line Interface
 *  @ingroup ICOV
 */

/** @file EncodeDemoMode.h
 *  @brief BRIEF_DESCRIPTION
 *  DESCRIPTION
 *  @ingroup CLI
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.5.0
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

#include "icov_common.h"
#include "encoder/InputMedia.h"
#include "encoder/interface/IEncoder.h"
#include "ldpc_codec_layer/factory/LdpcCoderParameters.h"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
namespace fs = boost::filesystem;
namespace po = boost::program_options;

class EncodeDemoMode {
  struct params {

    static const char *inputFile_id;
    static const char *outputFile_id;
    static const char *icuSize_id;

    fs::path inputFile;
    fs::path encodedFile;
    int icuSize;
  };

public:
  EncodeDemoMode(const po::variables_map &args);
  ~EncodeDemoMode() = default;

  std::string name() const { return "encode-demo"; }

  void run();

  static void AddProgramOptions(po::options_description &desc);

private:
  int encode();
  std::unique_ptr<icov::encoder::IEncoder<cv::Mat>> m_encoder;
  params m_parameters;
};
