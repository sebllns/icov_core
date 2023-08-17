/****************************************************************************/
/** @addtogroup CLI
 *  Command Line Interface
 *  @ingroup ICOV
 */

/** @file InteractiveDecoder.h
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

#include "InteractiveModeBase.h"
#include "app_common/decoder/SinglePassDecoder.h"
#include "decoder/DecodeResult.h"
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/map.hpp>
#include <fstream>
#include <memory>

namespace fs = boost::filesystem;
namespace po = boost::program_options;

class InteractiveDecoder : public InteractiveModeBase
{
  public:
    struct params
    {
        bool dummyViewport;
        float viewportPadding;
        fs::path icov_file;
        params(const po::variables_map &args);
    };

    InteractiveDecoder(const params &params, const LdpcCoderParameters &ldpc_params);

  private:
    params m_params;
    std::shared_ptr<icov::decoder::DecodeResult> m_results;
    std::unique_ptr<icov::decoder::SinglePassDecoder> m_decoder;

    // InteractiveModeBase interface
  protected:
    void updateF1() override;
    void updateF2() override;
    void updateF3() override;
    void updateF4() override;
    void updateF5() override;
    bool processKey(char key) override;
};
