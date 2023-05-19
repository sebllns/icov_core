/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup Decoder
 *  Decoder package (client side).
 *  @ingroup ICOV
 */

/** @file ChannelDecoder.h
 *  @brief Class representing a channel decoder for video encoding.
 *  @ingroup Decoder
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
#include "ldpc_codec_layer/BitSymbolProvider.h"
#include "ldpc_codec_layer/factory/LdpcCoderParameters.h"
#include "ldpc_codec_layer/factory/aff3ct_coder_types.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "video_layer/Encodingstructure.h"
#include "video_layer/ICOV_Quant.h"
#include "video_layer/ICOV_Transform2d.h"
#include "video_layer/extracted_stream.h"
#include "video_layer/icov_stream.h"
#include <map>
#include <memory>
#include <opencv2/opencv.hpp>

namespace icov {
namespace decoder {

/**
 * @class ChannelDecoder
 * @brief Class representing a channel decoder for video encoding.
 *
 * The ChannelDecoder class performs decoding operations on video bitstreams.
 * It is responsible for decoding ICU (Intra Coding Unit) blocks, setting SI
 * (Side Information) parameters, reading and managing syndrome information, and
 * decoding bitplanes.
 */
class ChannelDecoder {
public:
  /**
   * @struct params
   * @brief Struct containing the parameters for the ChannelDecoder.
   */
  struct params {
    icov::video::QpParam QpParam; /**< QP (Quantization Parameter) parameter. */
    ComponentID CompID;           /**< Component ID. */
    int bit_depth;                /**< Bit depth. */
    int maxLog2TrDynamicRange;    /**< Maximum Log2 Transform Dynamic Range. */
  };

  /**
   * @brief Constructor for ChannelDecoder.
   * @param p The parameters for the ChannelDecoder.
   * @param LDPC_params The parameters for the LDPC (Low Density Parity Check)
   * coder.
   * @param vvc_bufs Shared pointer to the video buffer groups.
   * @param encodingStruct Shared pointer to the encoding structure.
   * @param extraction Shared pointer to bitstream used to read the extracted
   * information from it.
   */
  ChannelDecoder(
      const ChannelDecoder::params &p, const LdpcCoderParameters &LDPC_params,
      const std::shared_ptr<icov::video::BufGroups> &vvc_bufs,
      const std::shared_ptr<icov::video::EncodingStructure> &encodingStruct,
      const bitstream::sp_IcovExtractedBitstream &extraction);
  ~ChannelDecoder();

  /**
   * @brief Decode an ICU (Interactive Coding Unit) block.
   * @param icu_enc The ID of ICU block to be decoded.
   * @param block_prediction The type of block prediction used.
   */
  void decode_icu(
      int icu_enc,
      omnidirectional_layer::BlockPrediction::ePredType block_prediction);

  /**
   * @brief Set the SI (Side Information) direction mode.
   * @param dir_mode The direction mode to be set.
   */
  void set_si_dir_mode(int dir_mode);

  void set_access_block_params(int lx);

  void set_si_params(int zmin, int zmax, float q, int lx);

  /**
   * @brief Read the syndrome from extracted bitstream.
   * @param syndrome_size The size of the syndrome.
   */
  void read_syndrome(int syndrome_size);

  void decode_bitplane(int bp, NumcodeIdx numcode, const Syndrome &synd);

  void end_decode_icu();

  Syndrome &syndrome();

  const ldpc_codec_layer::AffInteractiveCoder &ldpc_coder() const;

private:
  params m_params;
  std::shared_ptr<icov::video::BufGroups>
      m_vvc_bufs; ///< Shared pointer to the video buffer groups.
  std::shared_ptr<icov::video::EncodingStructure>
      m_encodingStruct; ///< Shared pointer to the encoding structure.
  std::unique_ptr<ldpc_codec_layer::AffInteractiveCoder>
      m_ldpc_coder; ///< Pointer to the LDPC (Low Density Parity Check) coder.
  icov::video::Quant m_quant;
  TCoeff m_uiAbsSum;
  icov::video::Transform2D m_transform2D;
  icov::video::IntraPrediction m_intraPred;
  BitVec m_blockBitplane;
  SIVec m_blockSI;
  Syndrome m_syndrome;
  ldpc_codec_layer::BitSymbolProvider m_bsp;
  omnidirectional_layer::BlockPrediction::ePredType m_block_prediction;
  bitstream::sp_IcovExtractedBitstream m_bitstream;
  bitstream::QaryParams m_qaryParams;
  int m_icuIndex;
  void _extract_bitplanes(omnidirectional_layer::BlockPrediction::ePredType block_prediction, int bitplaneCount);
  void _extract_bitplane(int i, omnidirectional_layer::BlockPrediction::ePredType block_prediction);
  void _extractIntraPredParams();
};
} // namespace decoder
} // namespace icov
