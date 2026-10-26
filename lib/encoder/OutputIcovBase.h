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

#include "Media.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include "video_layer/ContentParameterSet.h"
#include "video_layer/icov_stream.h"


#include <array>
#include <cstdint>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <ostream>
#include <vector>

namespace icov {
namespace encoder {

class EncoderStatistics;

/// Encoder output: the stream goes to one ICOV file, statistics and debug
/// information to side files next to it
class OutputIcovBase : public icov::IOutputIcov {
public:
  using rate_t = icov::bitstream::rate_t;
  explicit OutputIcovBase(const fs::path &path);
  virtual ~OutputIcovBase() = default;

  const bitstream::ContentParameterSet &contentParameterSet() const;
  bitstream::ContentParameterSet &contentParameterSet();

  virtual void startWriteBlock(const omnidirectional_layer::Block &b) = 0;
  virtual void startWritePrediction(
      omnidirectional_layer::IBlockPredictionManager::prediction_t pred) = 0;
  virtual void endWritePrediction(
      omnidirectional_layer::IBlockPredictionManager::prediction_t pred) = 0;
  virtual void endWriteBlock(const omnidirectional_layer::Block &b) = 0;

  virtual void
  writeQaryParams(const ldpc_codec_layer::QaryDistribution &bsp) = 0;
  virtual void writeSyndrome(const Syndrome &s) = 0;
  virtual void writeFrameSize() = 0;
  virtual void writeBlockSize() = 0;
  virtual void writeBitplaneCount(const uint &v) = 0;
  virtual void writeStoredSyndromeSize(const NumcodeIdx &v, const Rate &r,
                                       const uint &m) = 0;
  virtual void writeSIRate(const NumcodeIdx &v, const Rate &r,
                           const uint &m) = 0;
  virtual void writeSIMode(const uint &v) = 0;
  virtual void writeMse(const float &mse) = 0;
  /// CRC-32 of a reconstructed block, written after its MSE
  virtual void writeBlockHash(uint32_t crc) = 0;
  virtual void endIcov() = 0;

  /// Writes the ICOV file and the statistics file; call once, after encoding
  virtual void close() = 0;

  /// Human-readable encoder information, written to the statistics file
  void setInfo(const std::string &key, const std::string &value);
  void setStatistics(const EncoderStatistics &stats);

  /// Payload of the optional 'ildp' box (LDPC matrices)
  void setMatrixSet(std::vector<uint8_t> payload) {
    m_matrixSet = std::move(payload);
  }

  /// Debug text output (ExportDebugTextFormat), opened on first use
  std::ostream &debugStream();

  int channel() const;
  void setChannel(int newChannel);

  /// Side files: <name>.stats.json, <name>.ref/ (reference frames)
  fs::path statsPath() const;
  fs::path referenceDir() const;

protected:
  bitstream::ContentParameterSet m_contentParameterSet;
  std::map<std::string, std::string> m_info;
  std::unique_ptr<EncoderStatistics> m_stats;
  std::vector<uint8_t> m_matrixSet;

private:
  std::unique_ptr<std::ofstream> m_debug;
  int m_channel;
};

} // namespace encoder
} // namespace icov
