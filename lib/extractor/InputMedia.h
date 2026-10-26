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

#include "FastArray.h"
#include "Media.h"
#include "omnidirectional_layer/Block.h"
#include "video_layer/ContentParameterSet.h"
#include "video_layer/extracted_stream.h"
#include "video_layer/icov_stream.h"
#include "video_layer/bit_io.h"
#include "video_layer/block_syntax.h"
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <istream>
#include <memory>
#include <ostream>
#include <sys/types.h>
#include <vector>

namespace icov {
namespace extractor {

using pstream = std::unique_ptr<std::istream>;

/// Reader of a single-file ICOV stream (server role): parses the parameter
/// set and the block index, then loads one storage block unit (SBU) at a time
class InputIcov : public icov::IInputIcov<pstream> {
public:
  using rate_t = icov::bitstream::rate_t;

  InputIcov(const fs::path &path);
  ~InputIcov();

  const bitstream::ContentParameterSet &contentParameterSet() const;
  bitstream::ContentParameterSet &contentParameterSet();

  /// Raw parameter set, as sent to a client
  const std::vector<uint8_t> &parameterSetBytes() const;

  void setFrame(frame_t frame) override;
  void selectBlock(const omnidirectional_layer::Block::id_t &id);

  /// Parameters of the selected block, see block_syntax
  bitstream::BitReader &params() { return m_params; }

  /// Skips the parameters of one prediction of one channel
  void jumpPrediction(int bitplaneCount);

  /// Copies read_size bytes of syndrome, then skips to the end of the
  /// stored syndrome (stored_size bytes in total)
  void readSyndrome(byte *output, uint64_t read_size, uint64_t stored_size);
  void offsetSyndrome(uint offset);

  /// Encoder MSE of the selected block, NaN without statistics file
  float blockMse() const;

  /// CRC-32 of each reconstructed block, per frame and block in encoding
  /// order; empty when the file has no ihsh box
  const std::vector<std::vector<uint32_t>> &blockHashes() const {
    return m_hash;
  }

  omnidirectional_layer::Block::id_t
  getBlockIndexInStream(const omnidirectional_layer::Block::id_t &id) const;

  void resetStreams();

  void writeInfos(std::ofstream &output, int key, void *param) override;

private:
  struct IndexEntry {
    uint64_t offset;
    uint32_t params_size;
    uint32_t syndrome_size;
  };

  void readStatistics();

  bitstream::ContentParameterSet m_contentParameterSet;
  std::vector<uint8_t> m_ips;
  std::ifstream m_file;
  std::vector<IndexEntry> m_index; ///< per frame, per coded block
  size_t m_blockCount;             ///< coded blocks per frame

  std::vector<uint8_t> m_sbu; ///< selected storage block unit
  size_t m_blockPos;          ///< index of the selected block in its frame
  bitstream::BitReader m_params;
  size_t m_syndromePos; ///< read position in the syndrome part of m_sbu

  std::vector<std::vector<float>> m_mse; ///< per frame, per block
  std::vector<std::vector<uint32_t>> m_hash;
};

} // namespace extractor
} // namespace icov
