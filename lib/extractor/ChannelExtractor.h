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

#include "extractor/InputMedia.h"
#include "icov_types.h"
#include "ldpc_codec_layer/factory/LdpcCoderParameters.h"
#include "ldpc_codec_layer/factory/aff3ct_coder_types.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "video_layer/extracted_stream.h"
#include <array>
#include <map>
#include <memory>
#include <sys/types.h>
#include <vector>

#define MAX_BITPLANE_COUNT 10

#define PRED_COUNT 3
#define S_STORED 0
#define S_INTER 1
#define S_INTRA 2

namespace icov {
namespace extractor {

struct BitplaneSyndromeExtractor {
  ushort Index;
  bitstream::RateStream BitRates[PRED_COUNT];
  ushort ReadSyndromePadding;
};

/**
 * @brief The ChannelExtractor class represents an object responsible for
 * extracting the channel layer data from the compressed stream and decoding it.
 */
class ChannelExtractor {
public:
  /**
   * @brief The params struct represents a structure to store the component ID
   * associated with the channel extractor.
   */
  struct params {
      int8b CompID;
  };

  /**
   * @brief Construct a new Channel Extractor object.
   * @param p The parameters used to initialize the object
   * @param icovFile The input file object
   * @param bitstream The extracted bitstream output
   * @param LDPC_params The parameters of the LDPC encoder/decoder used in the
   * channel extraction
   */
  ChannelExtractor(const ChannelExtractor::params &p,
                   std::shared_ptr<InputIcov> &icovFile,
                   const bitstream::sp_IcovExtractedBitstream &bitstream,
                   const LdpcCoderParameters &LDPC_params);
  ~ChannelExtractor();

  /**
   * @brief Get the coder size
   * @return The size of the coder
   */
  int getCoderSize() const;

  // void extract_access_block(const omnidirectional_layer::Block &b,
  //                        byte *syndrome_buf);

  void extract_block_syndrome(const omnidirectional_layer::Block &b,
                              omnidirectional_layer::BlockPrediction::ePredType block_prediction);

  void extract_stored_syndrome_size(bool is_access_block);

  void pass_access_block();
  void pass_prediction();
  void extractPredParams(int type);

  // const BlockChannelDecoderParams &extractedParams() const;

  static void resetSyndromePadding();
  static int getSyndromePadding();

  const std::array<bitstream::SIEncodedParams, PRED_COUNT> &predParams() const;

private:
  void _extractPredParams(bitstream::SIEncodedParams &si);
  void _pushPredParams(int type);
  void _pushPredParams(const bitstream::SIEncodedParams &si);
  void _extractBitplaneNumcode(BitplaneSyndromeExtractor &bp, int pred_type);

  /**
   * @brief Extract the bitplane syndrome
   * @param bp The bitplane to extract the syndrome from
   */
  void _extractBitplaneSyndrome(const BitplaneSyndromeExtractor &bp,
                                int pred_type);

  params m_params; ///< The parameters of the channel extractor

  std::unique_ptr<ldpc_codec_layer::AffInteractiveCoder>
      m_ldpc_coder; ///< A pointer to the LDPC coder

  std::shared_ptr<InputIcov> m_icovFile; ///< The input file

  bitstream::sp_IcovExtractedBitstream m_bitstream;
  std::vector<BitplaneSyndromeExtractor> m_bitplanes;
  byte m_bitplaneCount;
  int m_maxSyndromeBufferLength;
  // bool m_codingFlag;
  std::array<bitstream::SIEncodedParams, PRED_COUNT> m_predParams;

  static byte SyndromePadding;
};
} // namespace extractor
} // namespace icov
