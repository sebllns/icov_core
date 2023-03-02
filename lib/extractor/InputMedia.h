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

#include "Block.h"
#include "Media.h"
#include "common_types.h"
#include "encoded_stream.h"
#include "encoded_stream/ContentParameterSet.h"
#include "extracted_stream.h"
#include "icov_stream.h"
#include "typedef.h"
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

  const icov::ContentParameterSet &contentParameterSet() const;
  icov::ContentParameterSet &contentParameterSet();
  void readContentParameterSet();

  void initBuffer(size_t block_count, size_t bufferSize);

  void offsetSyndrome(uint offset);

  template <class T>
  void readSyndrome(T &output, uint64_t read_size, uint64_t stored_size) {
    output.resize(read_size);
    auto byte_count = bitToByteCount(read_size);
    auto &str = getStream(eIcovStream::SYNDROME);
    icov::helpers::io::read_value(str, m_readBuf, byte_count);
    m_syndromeOffset = read_bits(output, m_readBuf, m_syndromeOffset);
    byte_count = bitToByteCount(stored_size) - byte_count;
    if (m_syndromeOffset)
      --byte_count;
    str.seekg(byte_count, std::ios_base::cur);
  }

  void readSyndrome(icov::byte *output, uint64_t read_size,
                    uint64_t stored_size);
  void readFrameSize();
  void readBlockPos();
  void selectBlock(size_t id);
  void readSyndromesSizes(std::vector<int> &bitplaneCount,
                          std::vector<NumcodeIdxVec> &output);
  void readSyndromesSizes(NumcodeIdxVec &output);
  void selectPrediction(ushort pred_idx, const std::vector<int> &bitplaneCount);

  template <typename T> void read(int id, int size, T *output) {
    icov::helpers::io::read_value(getStream(id), output, size);
  }

  template <typename T> void read(int id, T &output) {
    icov::helpers::io::read_value(getStream(id), &output, 1);
  }

  int readByteIntParam();
  void readQaryParams(byte header, int &q_idx, int &z_min, int &z_max);
  void readSIRates(NumcodeIdxVec &v);
  void extractBlock(BlockChannelDecoderParams &output, int icu_idx);

  void writeInfos(std::ofstream &output, int key, void *param) override;
  void setFrame(int frame) override;
  std::istream &getStream(int id);

  void passPrediction(int bitplaneCount);

private:
  icov::ContentParameterSet m_contentParameterSet;
  boost::dynamic_bitset<> m_syndromes;
  std::map<int, pstream> m_fileParts;

  int syndrome_bitshift;
  int last_bit_shift;
  icov::byte last_byte;

  size_t m_blockCount;

  std::vector<std::streampos> m_rates_pos;
  std::vector<std::streampos> m_synd_pos;

  bitstream::QaryParams m_qaryParams;

  icov::byte *m_readBuf;

  ushort m_syndromeOffset;
};

} // namespace extractor
} // namespace icov
