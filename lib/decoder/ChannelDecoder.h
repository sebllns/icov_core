/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup ChannelDecoder
 * ChannelDecoder package (client side).
 *  @ingroup ICOV
 */

/** @file ChannelDecoder.h
 *  @brief Defines an actual ICOV ChannelDecoder.
 *  @ingroup ChannelDecoder
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

#include "Unit.h"
#include "common_types.h"
#include "ldpc_codec_layer/BitSymbolProvider.h"
#include "ldpc_codec_layer/factory/LdpcCoderParameters.h"
#include "ldpc_codec_layer/factory/aff3ct_coder_types.h"
#include "omnidirectional_layer/BlockPrediction.h"
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
class ChannelDecoder {
  // IBlockProcessor interface
public:
  struct params {
    icov::video::QpParam QpParam;
    ComponentID CompID;
    int bit_depth;
    int maxLog2TrDynamicRange;
  };

  ChannelDecoder(
      const ChannelDecoder::params &p, const LdpcCoderParameters &LDPC_params,
      const std::shared_ptr<icov::video::BufGroups> &vvc_bufs,
      const std::shared_ptr<icov::video::EncodingStructure> &encodingStruct,
      const bitstream::sp_IcovExtractedBitstream &extraction);
  ~ChannelDecoder();

  void decode_icu(int icu_enc, BlockPrediction::predType block_prediction);

  void set_si_dir_mode(int dir_mode);

  void set_access_block_params(int lx);

  void set_si_params(int zmin, int zmax, float q, int lx);

  void read_syndrome(int syndrome_size);

  void decode_bitplane(int bp, NumcodeIdx numcode, const Syndrome &synd);

  void end_decode_icu();

  Syndrome &syndrome();

  const AffInteractiveCoder &ldpc_coder() const;

protected:
  params m_params;
  std::shared_ptr<icov::video::BufGroups> m_vvc_bufs;
  std::shared_ptr<icov::video::EncodingStructure> m_encodingStruct;
  // CompArea m_blockArea;
  std::unique_ptr<AffInteractiveCoder> m_ldpc_coder;
  icov::video::Quant quant;
  TCoeff uiAbsSum;
  icov::video::Transform2D transform2D;
  icov::video::IntraPrediction intraPred;
  BitVec m_blockBitplane;
  SIVec m_blockSI;
  Syndrome m_syndrome;
  BitSymbolProvider m_bsp;
  BlockPrediction::predType m_block_prediction;
  bitstream::sp_IcovExtractedBitstream m_bitstream;
  bitstream::QaryParams m_qaryParams;
  // BlockVector::const_iterator m_block;
  int m_icuIndex;
  // icov::video::IcuEncoder *mp_icu_enc;
  // PelBuf roiComp_predictionFrameBuff;
};
} // namespace decoder
} // namespace icov
