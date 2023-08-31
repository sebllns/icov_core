/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup IcovEncoder
 * IcovEncoder package (client side).
 *  @ingroup ICOV
 */

/** @file IcovEncoder.h
 *  @brief Defines an actual ICOV IcovEncoder.
 *  @ingroup IcovEncoder
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

#include "app_common/program/icov_program_options.h"
#include "encoder/IcovMediaEncoder.h"
#include "ldpc_codec_layer/QaryDistribution.h"
#include <boost/filesystem/path.hpp>
#include <memory>
#include <string>

using namespace icov::omnidirectional_layer;

namespace icov {
namespace encoder
{
class IcovEncoder
{
  public:
    struct params
    {
        std::string m_inputfile;
        std::string m_outputfile;
        uint m_debugframecount;
        uint m_qp;
        uint m_icuSize;
        float fov_x_degree;
        float viewport_padding;
        point2d resolution;
        angle_radian fov;
        bool m_DummyViewport;
        ldpc_codec_layer::QaryDistribution::quantization_params bsp_params;
        params(const options::args_t &args);
    };

    IcovEncoder(const params &params, const LdpcCoderParameters &ldpc_params);
    ~IcovEncoder();
    void run();

    const LdpcCoderParameters &ldpc_params() const;
    const params &getParams() const;
    const fs::path &inputfile() const;
    const fs::path &outputfile() const;

  private:
    params m_params;
    std::unique_ptr<LdpcCoderParameters> m_ldpc_params;
    std::unique_ptr<encoder::IcovMediaEncoder> m_encoder;
};
} // namespace encoder
} // namespace icov
