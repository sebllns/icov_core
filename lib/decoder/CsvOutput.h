/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup Decoder
 *  Decoder package (client side).
 *  @ingroup ICOV
 */

/** @file CsvOutput.h
 *  @brief Defines an actual ICOV CsvOutput.
 *  @ingroup Decoder
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

#include "decoder/InfosGatherer.h"
#include "omnidirectional_layer/interface/IBlockProcessor.h"
#include <fstream>
#include <memory>
#include <vector>

namespace icov
{
namespace decoder {
class CsvOutput : public omnidirectional_layer::IBlockProcessor
{
  public:
    CsvOutput(const std::string &path_prefix);

  protected:
    const std::shared_ptr<InfosGatherer> getParent();
    std::shared_ptr<InfosGatherer> m_decoderParent;

    std::ofstream m_requestFile;
    std::ofstream m_frameFile;
    std::ofstream m_blockFile;

    unsigned long m_requestID;

    // IBlockProcessor interface
    void _beforeOrderingLoop(const request_t &r) override;
    void _afterOrderingLoop(const request_t &r) override;
    void _processBlock(const request_t &r, const omnidirectional_layer::Block &b,
                       omnidirectional_layer::IBlockPredictionManager::prediction_t prediction) override;
};
} // namespace decoder
} // namespace icov
