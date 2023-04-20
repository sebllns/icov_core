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

#include "Media.h"
#include "common_types.h"
#include "ldpc_codec_layer/BitSymbolProvider.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include "video_layer/ContentParameterSet.h"
#include "video_layer/icov_stream.h"
#include <array>
#include <boost/dynamic_bitset.hpp>
#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <cstdint>
#include <fstream>
#include <memory>
#include <ostream>
#include <vector>

namespace icov {
namespace encoder {

class OutputIcovBase : public icov::IOutputIcov {
public:
  enum eWriteCounter {
    BLOCK_RATE = 0,
    BLOCK_SYNDROME,
    FRAME_RATE,
    FRAME_SYNDROME,
    TOTAL_WRITE,
    TOTAL_LOST,
    COUNT
  };

  using rate_t = icov::bitstream::rate_t;
  explicit OutputIcovBase(const fs::path &path, int openflag);
  ~OutputIcovBase() = default;

  const bitstream::ContentParameterSet &contentParameterSet() const;
  bitstream::ContentParameterSet &contentParameterSet();

  virtual void startWriteBlock(const omnidirectional_layer::Block &b) = 0;
  virtual void startWritePrediction(
      omnidirectional_layer::IBlockPredictionManager::prediction_t pred) = 0;
  virtual void endWritePrediction(
      omnidirectional_layer::IBlockPredictionManager::prediction_t pred) = 0;
  virtual void endWriteBlock(const omnidirectional_layer::Block &b) = 0;

  virtual void
  writeQaryParams(const ldpc_codec_layer::BitSymbolProvider::PzParams &bsp) = 0;
  virtual void writeContentParameterSet() = 0;
  virtual void writeSyndrome(const Syndrome &s) = 0;
  virtual void writeFrameSize() = 0;
  virtual void writeBlockSize() = 0;
  virtual void writeBitplaneCount(const uint &v) = 0;
  virtual void writeStoredSyndromeSize(const uint &v) = 0;
  virtual void writeSIRate(const uint &v) = 0;
  virtual void writeSIMode(const uint &v) = 0;
  void flush(int id);
  std::ostream &getStream(int id);

protected:
  bitstream::ContentParameterSet m_contentParameterSet;
  std::map<int, std::unique_ptr<std::ostream>> m_fileParts;
  std::array<uint32_t, eWriteCounter::COUNT> m_total_write_byte;
};

} // namespace encoder
} // namespace icov
