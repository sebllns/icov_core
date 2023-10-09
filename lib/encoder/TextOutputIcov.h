/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup Encoder
 *  ICOV Encoder Lib
 *  @ingroup ICOV
 */

/** @file SourceMedia.h
 *  @brief Defines an actual ICOV SourceMedia.
 *  @ingroup SourceMedia
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

#include "FastArray.h"
#include "Media.h"
#include "OutputIcovBase.h"
#include "ldpc_codec_layer/BitSymbolProvider.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include "video_layer/icov_stream.h"

#include <boost/dynamic_bitset.hpp>
#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>

#include <cstdint>
#include <fstream>
#include <functional>
#include <memory>
#include <ostream>
#include <vector>

namespace icov {
namespace encoder {

class TextOutputIcov : public OutputIcovBase {
public:
  using rate_t = icov::bitstream::rate_t;
  explicit TextOutputIcov(const fs::path &path);
  ~TextOutputIcov() = default;

  // OutputIcovBase interface
  void writeQaryParams(const ldpc_codec_layer::QaryDistribution &bsp) override;
  void writeContentParameterSet() override;
  void writeSyndrome(const Syndrome &s) override;
  void writeFrameSize() override;
  void writeBlockSize() override;
  void writeBitplaneCount(const uint &v) override;
  void writeStoredSyndromeSize(const NumcodeIdx &v, const Rate &r,
                               const uint &m) override;
  void writeSIRate(const NumcodeIdx &n, const Rate &r, const uint &m) override;
  void writeSIMode(const uint &v) override;
  void startWriteBlock(const omnidirectional_layer::Block &b) override;
  void startWritePrediction(
      omnidirectional_layer::IBlockPredictionManager::prediction_t pred)
      override;
  void endWritePrediction(
      omnidirectional_layer::IBlockPredictionManager::prediction_t pred)
      override;
  void endWriteBlock(const omnidirectional_layer::Block &b) override;

private:
  struct _blockNode {
    boost::json::object nodeJSON;
    std::function<boost::json::object()> initChannelNode;

    void init(const omnidirectional_layer::Block &b) {
      nodeJSON.clear();
      nodeJSON["block_id"] = b.getIdSelf();
      nodeJSON["is_access_block"] = false;
      nodeJSON["channels"] = boost::json::object();
    }

    boost::json::object &getChannelNodeJSON(int ch) {

      // create this channel if not exist
      if (!nodeJSON["channels"].as_object()[std::to_string(ch)].is_object()) {

        nodeJSON["channels"].as_object()[std::to_string(ch)] =
            initChannelNode();
      }

      // return the channel
      return nodeJSON["channels"].as_object()[std::to_string(ch)].as_object();
    }
  };

  _blockNode m_blockNodeJSON;
  boost::json::object m_currentPrednodeJSON;
  _blockNode m_syndromeNodeJSON;

  //  boost::json::object &getChannelNodeJSON();

  //  void _initNode(boost::json::object &m_blockNodeJSON,
  //                 const omnidirectional_layer::Block &b);

  // OutputIcovBase interface
public:
  void writeMse(const float &mse) override;
};

} // namespace encoder
} // namespace icov
