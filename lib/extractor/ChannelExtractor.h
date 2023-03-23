/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup ChannelExtractor
 * ChannelExtractor package (client side).
 *  @ingroup ICOV
 */

/** @file ChannelExtractor.h
 *  @brief Defines an actual ICOV ChannelExtractor.
 *  @ingroup ChannelExtractor
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
#include "BlockPrediction.h"
#include "Media.h"
#include "common_types.h"
#include "extracted_stream.h"
#include "extractor/InputMedia.h"
#include "ldpc_codec_layer/IcovCoder.h"
#include "typedef.h"
#include <map>
#include <memory>
#include <sys/types.h>
#include <vector>

#define MAX_BITPLANE_COUNT 20

namespace icov {
namespace extractor {

struct BitplaneExtractor {
  icov::byte StoredSyndromeQuantizedSize;
  ushort StoredSyndromeBitSize;
  ushort StoredSyndromeByteSize;

  icov::byte ReadSyndromeQuantizedSize;
  ushort ReadSyndromeBitSize;
  ushort ReadSyndromeByteSize;
  ushort ReadSyndromePadding;
};

class ChannelExtractor {
public:
  //  struct params {
  //    icov::int8b ChannelType;
  //    icov::int8b ChromaFomat;
  //    icov::int8b CompID;
  //  };

  ChannelExtractor(/*const ChannelExtractor::params &p,*/
                   std::shared_ptr<InputIcov> &icovFile,
                   const sp_IcovExtractedBitstream &bitstream,
                   const LdpcCoderParameters &LDPC_params);
  ~ChannelExtractor();

  int get_coder_size() const;

  void extract_access_block(const Block &b, icov::byte *syndrome_buf);

  void extract_block(const Block &b,
                     BlockPrediction::predType block_prediction);

  void extract_syndrome_sizes(bool is_access_block);

  void pass_access_block();
  void pass_prediction();

  // const BlockChannelDecoderParams &extractedParams() const;

  static void resetSyndromePadding();
  static int getSyndromePadding();

protected:
  void extract_bitplane_syndrome(const BitplaneExtractor &bp);

  /*params m_params;*/
  std::unique_ptr<AffInteractiveCoder> m_ldpc_coder;
  std::shared_ptr<InputIcov> m_icovFile;
  sp_IcovExtractedBitstream m_bitstream;
  std::vector<BitplaneExtractor> m_bitplanes;
  icov::byte m_bitplaneCount;
  // BlockChannelDecoderParams m_extractData;
  int m_maxSyndromeBufferLength;
  //  RawByteArray<int> m_readingBuffer;
  static icov::byte SyndromePadding;
};
} // namespace extractor
} // namespace icov
