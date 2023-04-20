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

#include "Buffer.h"
#include "decoder/BlockDecoder.h"
#include "extractor/BlockExtractor.h"
#include "extractor/InputMedia.h"
#include "omnidirectional_layer/factory.h"
#include "omnidirectional_layer/interface/IBlockProcessor.h"
#include <memory>
#include <queue>

using namespace icov;
using namespace icov::extractor;
using namespace icov::omnidirectional_layer;
using namespace icov::ldpc_codec_layer;
using namespace icov::bitstream;

namespace icov {
namespace decoder {
class SinglePassDecoder {
public:
  struct params {};

  SinglePassDecoder(const fs::path &path);
  ~SinglePassDecoder();

  void initialize(const LdpcCoderParameters &coder_params);

  const InputIcov &icovFile() const;
  const factory::Navigation360 &icovNavigation() const;
  IBlockProcessor &blockProcessor();
  const PelUnitBuf getOutputBuf() const;
  void decodeFullFrame();

protected:
  struct BlockProcessor : public IBlockProcessor {
    std::unique_ptr<BlockExtractor> Extractor;
    std::unique_ptr<BlockDecoder> Decoder;
    bitstream::sp_IcovExtractedBitstream ExtractionBitstream;

    void
    processBlock(const icov::omnidirectional_layer::Block &b,
                 omnidirectional_layer::IBlockPredictionManager::prediction_t
                     prediction) override {
      if (Extractor)
        Extractor->processBlock(b, prediction);
      if (Decoder)
        Decoder->processBlock(b, prediction);
    }
    void startProcess() override {
      if (Extractor)
        Extractor->startProcess();
      if (Decoder)
        Decoder->startProcess();
    }
    void endProcess() override {
      if (Extractor)
        Extractor->endProcess();
      if (Decoder)
        Decoder->endProcess();

      icov::helpers::assert::equal(ExtractionBitstream->remains(), 0,
                                   "Buffer was not read in totality");
    }
  };

  std::vector<float> m_q_values;
  std::shared_ptr<InputIcov> m_icovFile;
  bitstream::sp_IcovExtractedBitstream m_extractionBitstream;
  factory::Navigation360 m_navigation;
  BlockProcessor m_blockProcessor;
};
} // namespace decoder
} // namespace icov
