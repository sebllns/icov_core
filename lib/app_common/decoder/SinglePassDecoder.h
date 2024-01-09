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

#include "Buffer.h"
#include "decoder/BlockDecoder.h"
#include "extractor/BlockExtractor.h"
#include "extractor/InputMedia.h"
#include "icov_request.h"
#include "omnidirectional_layer/factory.h"
#include "omnidirectional_layer/interface/IBlockProcessor.h"
#include "video_layer/ContentParameterSet.h"

#include <memory>
#include <queue>
#include <vector>

using namespace icov;
using namespace icov::extractor;
using namespace icov::omnidirectional_layer;
using namespace icov::ldpc_codec_layer;
using namespace icov::bitstream;

namespace icov {
namespace decoder {

class SinglePassDecoder
{
  public:
    struct params
    {
        bool DummyViewport = false;
        fs::path IcovFile;
        fs::path LdpcDatabase;
    };

    SinglePassDecoder();
    ~SinglePassDecoder();

    void initLocal(const params &p);

    void openIcovFile(const fs::path &path);
    void addGroundTruth(const fs::path &path);

    void addDefaultNavigation();
    void addDefaultExtractor();
    void addDefaultDecoder();
    const PelUnitBuf getDecodedBuf() const;

    const std::shared_ptr<BlockExtractor> &blockExtractor() const;
    const std::shared_ptr<BlockDecoder> &blockDecoder() const;
    const std::shared_ptr<IBlockProcessor> &blockProcessor() const;

    void setBlockExtractor(const std::shared_ptr<BlockExtractor> &newBlockExtractor);
    void setBlockDecoder(const std::shared_ptr<BlockDecoder> &newBlockDecoder);

    void start();
    bool request(const request_t &r);

    const LdpcCoderParameters &ldpcParams() const;
    void setLdpcParams(const LdpcCoderParameters &newLdpcParams);

    const ContentParameterSet &contentParams() const;

    const std::shared_ptr<factory::Navigation360> &navigation() const;
    void setNavigation(const std::shared_ptr<factory::Navigation360> &newNavigation);

    const std::shared_ptr<InputIcov> &icovFile() const;
    void setIcovFile(const std::shared_ptr<InputIcov> &newIcovFile);

    const bitstream::sp_IcovExtractedBitstream &extractionBitstream() const;

    void setContentParams(const ContentParameterSet &newContentParams);

    const params &getParams() const;
    void setParams(const params &newParams);

  protected:
    params m_params;
    std::vector<float> m_q_values;
    ContentParameterSet m_contentParams;
    bitstream::sp_IcovExtractedBitstream m_extractionBitstream;
    std::shared_ptr<factory::Navigation360> m_navigation;
    std::unique_ptr<LdpcCoderParameters> m_ldpcParams;
    std::shared_ptr<InputIcov> m_icovFile;
    std::shared_ptr<IBlockProcessor> m_blockProcessor;
    std::shared_ptr<BlockExtractor> m_blockExtractor;
    std::shared_ptr<BlockDecoder> m_blockDecoder;

    request_t m_request;
    void catchError();
};
} // namespace decoder
} // namespace icov
