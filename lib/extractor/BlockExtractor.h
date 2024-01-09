/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup BlockExtractor
 * BlockExtractor package (client side).
 *  @ingroup ICOV
 */

/** @file BlockExtractor.h
 *  @brief Defines an actual ICOV BlockExtractor.
 *  @ingroup BlockExtractor
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

#include "extractor/ChannelExtractor.h"
#include "extractor/InputMedia.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/interface/IBlockProcessor.h"
#include "video_layer/extracted_stream.h"
#include <memory>
#include <queue>
#include <string>
#include <sys/types.h>
#include <vector>

namespace icov {
namespace extractor {

/**
 * @brief Class for extracting block data (i.e. ICU) from input encoded ICOV
 * file
 */
class BlockExtractor : public omnidirectional_layer::IBlockProcessor {
public:
  struct params {
    std::vector<int> icu_sizes;
  };

  BlockExtractor(std::shared_ptr<InputIcov> &icovFile,
                 const BlockExtractor::params &p,
                 const LdpcCoderParameters &coder_params,
                 const bitstream::sp_IcovExtractedBitstream &extraction);
  ~BlockExtractor();

protected:
  std::shared_ptr<InputIcov>
      m_icovFile; /**< Shared pointer to input ICOV file */

  bitstream::sp_IcovExtractedBitstream
      m_extractionBitstream; /**< Output extracted bitstream */

  int m_channelCount; /**< Number of channels */

  std::vector<std::unique_ptr<ChannelExtractor>>
      m_channelExtractor; /**< Vector of unique pointers to ChannelExtractor
                             objects */

  params m_params; /**< BlockExtractor parameters */

  // IBlockProcessor interface
protected:
  void _beforeOrderingLoop(const request_t &r) override;
  void _afterOrderingLoop(const request_t &r) override;
  void _processBlock(
      const request_t &r, const omnidirectional_layer::Block &b,
      omnidirectional_layer::IBlockPredictionManager::prediction_t prediction,
      frame_t previouslyDecoded) override;

private:
  void fill_metrics(
      icov::omnidirectional_layer::IBlockPredictionManager::prediction_t
          prediction,
      const omnidirectional_layer::Block &b);

  int block_count;
};
} // namespace extractor
} // namespace icov
