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
#include "omnidirectional_layer/Block.h"
#include "video_layer/ContentParameterSet.h"
#include "video_layer/extracted_stream.h"
#include "video_layer/icov_stream.h"
#include <boost/dynamic_bitset.hpp>
#include <boost/dynamic_bitset/dynamic_bitset.hpp>
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
class InputIcov : public icov::IInputIcov<pstream> {
public:
  using rate_t = icov::bitstream::rate_t;

  InputIcov(const fs::path &path);
  ~InputIcov();

  const bitstream::ContentParameterSet &contentParameterSet() const;
  bitstream::ContentParameterSet &contentParameterSet();
  void readContentParameterSet();

  void offsetSyndrome(uint offset);

  void readSyndrome(byte *output, uint64_t read_size,
                    uint64_t stored_size);
  void readFrameSize();
  void readBlockPos();
  void selectBlock(const omnidirectional_layer::Block::id_t &id);
  void readSyndromesSizes(std::vector<int> &bitplaneCount,
                          std::vector<NumcodeIdxVec> &output);
  void readSyndromesSizes(NumcodeIdxVec &output);
  void selectPrediction(ushort pred_idx, const std::vector<int> &bitplaneCount);

  template <typename T> void read(int id, int size, T *output) {
    icov::io::read_value(getStream(id), output, size);
  }

  template <typename T> void read(int id, T &output) {
    icov::io::read_value(getStream(id), &output, 1);
  }

  int readByteIntParam();
  void readQaryParams(byte header, int &q_idx, int &z_min,
                      int &z_max);
  void readSIRates(NumcodeIdxVec &v);
  void extractBlock(bitstream::BlockChannelDecoderParams &output, int icu_idx);

  void writeInfos(std::ofstream &output, int key, void *param) override;
  void setFrame(frame_t frame) override;

  omnidirectional_layer::Block::id_t
  getBlockIndexInStream(const omnidirectional_layer::Block::id_t &id) const;

  /**
   * @brief Get the input stream for a given ID
   * @param id The ID of the input stream
   * @return A reference to the input stream for the given ID
   */
  std::istream &getStream(int id);

  /**
   * @brief Jump the prediction for the given number of bitplanes
   * @param bitplaneCount The number of bitplanes to pass the prediction for
   */
  void jumpPrediction(int bitplaneCount);

private:
  bitstream::ContentParameterSet
      m_contentParameterSet; ///< The content parameter set for the input stream

  std::map<int, pstream>
      m_fileParts; ///< The map of input file streams for each ID

  size_t m_blockCount; ///< The number of blocks in the input stream

  std::vector<std::streampos>
      m_rates_pos; ///< The vector of stream positions that indicates where SI
                   ///< rates data start for each block in the input stream

  std::vector<std::streampos>
      m_synd_pos; ///< The vector of stream positions that indicates where
                  ///< syndrome data start for each block in the input stream

  bitstream::QaryParams m_qaryParams;
};

} // namespace extractor
} // namespace icov
