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
 *  @version 1.0.1
 *  @date 2022
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
#include "encoded_stream/ContentParameterSet.h"
#include "typedef.h"
#include <boost/dynamic_bitset.hpp>
#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <cstdint>
#include <fstream>
#include <memory>
#include <ostream>
#include <vector>

#ifndef ICOV_RATE_SIZE
#define ICOV_RATE_SIZE 1
#endif

namespace icov {
namespace encoder {

using rate_t =
#if ICOV_RATE_SIZE == 1
    icov::byte;
#elif ICOV_RATE_SIZE == 2
    uint16_t;
#else
    uint32_t;
#endif

class OutputIcov : public icov::IOutputIcov {
public:
  typedef enum {
    MEDIA_HEADER = 0,
    FRAMES_SIZE,
    BLOCKS_SIZE,
    RATES,
    SYNDROME,
    COUNT
  } eIcovStream;

  explicit OutputIcov(const fs::path &path);
  ~OutputIcov() = default;

  const icov::ContentParameterSet &contentParameterSet() const;
  icov::ContentParameterSet &contentParameterSet();
  void writeContentParameterSet();
  void writeSyndrome(bool nocoding, const Syndrome &s);
  void writeFrameSize();
  void writeBlockSize();
  void writeSIRates(const NumcodeIdxVec &v);
  void writeSIRate(const NumcodeIdx &v);

private:
  icov::ContentParameterSet m_contentParameterSet;
  boost::dynamic_bitset<> m_syndromes;
  std::map<int, std::unique_ptr<std::ostream>> m_fileParts;

  int syndrome_bitshift;
  int last_bit_shift;
  icov::byte last_byte;

  uint32_t m_total_write_byte_block[2];
  uint32_t m_total_write_byte_frame[2];

  int total_write_bits;
  int total_lost;
  int total_shift;

  int rates_count;
};

} // namespace encoder
} // namespace icov
