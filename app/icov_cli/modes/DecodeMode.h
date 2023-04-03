/****************************************************************************/
/** @addtogroup CLI
 *  Command Line Interface
 *  @ingroup ICOV
 */

/** @file DecodeMode.h
 *  @brief BRIEF_DESCRIPTION
 *  DESCRIPTION
 *  @ingroup CLI
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.0.3
 *  @date 2023
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

#include "BitSymbolProvider.h"
#include "decoder/SinglePassDecoder.h"
#include "encoder/IcovMediaEncoder.h"
#include "ldpc_codec_layer/factory/LdpcCoderParameters.h"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <memory>
namespace fs = boost::filesystem;
namespace po = boost::program_options;

class DecodeMode {
  struct params {
    static const char *InputFile_ID;
    static const char *IcuSize_ID;
    static const char *QP_ID;
    static const char *ViewportPadding_ID;

    fs::path input_file;
    int icu_size;
    int qp;
    float viewport_padding;
    std::unique_ptr<LdpcCoderParameters> ldpc_params;
    BitSymbolProvider::params bsp_params;
  };

public:
  DecodeMode(const po::variables_map &args);
  ~DecodeMode() = default;

  void run();
  void encode();
  void decode();
  void check();
  void display();

  static void AddProgramOptions(po::options_description &desc);

private:
  params m_parameters;
  std::unique_ptr<encoder::IcovMediaEncoder> m_encoder;
  std::unique_ptr<decoder::SinglePassDecoder> m_decoder;
};
