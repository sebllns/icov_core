/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup SinglePassDecoder
 * SinglePassDecoder package (client side).
 *  @ingroup ICOV
 */

/** @file SinglePassDecoder.h
 *  @brief Defines an actual ICOV SinglePassDecoder.
 *  @ingroup SinglePassDecoder
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

#include "extractor/ChannelExtractor.h"
#include "extractor/InputMedia.h"
#include "interface/IBlockProcessor.h"
#include <memory>
#include <queue>

using namespace icov::extractor;

namespace icov {
namespace decoder {
class SinglePassDecoder : public IBlockProcessor {
  // IBlockProcessor interface
public:
  struct params {};

  SinglePassDecoder(const fs::path &path);
  ~SinglePassDecoder();

  void initialize(const CoderParameters &coder_params);
  void startProcess() override;
  void processBlock(const Block &b, uint16_t prediction) override;
  void endProcess() override;

  const InputIcov &icovFile() const;

  protected:
  std::vector<float> m_q_values;
  std::shared_ptr<InputIcov> m_icovFile;
  sp_IcovExtractedBitstream m_extractionBitstream;
  std::unique_ptr<IBlockProcessor> m_extractor;
  std::unique_ptr<IBlockProcessor> m_decoder;
};
} // namespace decoder
} // namespace icov
