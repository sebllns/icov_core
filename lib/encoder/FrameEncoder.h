/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup Encoder
 *  ICOV Encoder Lib
 *  @ingroup ICOV
 */

/** @file FrameEncoder.h
 *  @brief Defines an actual ICOV FrameEncoder.
 *  @ingroup FrameEncoder
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

#include "Media.h"
#include "OutputIcovBase.h"
#include "PredictionsEncoder.h"
#include "common.h"
#include "encoder/interface/IEncoder.h"
#include "ldpc_codec_layer/BitSymbolProvider.h"

#include "ldpc_codec_layer/interface/IInteractiveCoder.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include "video_layer/Encodingstructure.h"
#include "video_layer/ExtractedStreamDebugger.h"

#include <opencv2/plot.hpp>

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace icov {
namespace encoder {
class FrameEncoder : public IFrameEncoder<cv::Mat> {
public:
  struct params {
    int QP = 42;
    ChromaFormat ChFormat = ChromaFormat::CHROMA_420;
    cv::Size ICU_Size = cv::Size(32, 32);
    ldpc_codec_layer::QaryDistribution::quantization_params BSP_Params;
    bool CheckDecoding = true;
    float MaxRateNoCoding = 3;
    // int maxCountSINoCoding;

    const Size TU_SizeAfterRegroupingLuma = Size(1, 1);
    const int MaxLog2TrDynamicRange = 15;

    /// in fact we need 1 margin for each color
    /// component, but since UnitArea
    /// handles division by 2 for chroma 420, it is easier to add 2 pixel
    /// margins per left and right side and this will be divided by 2 for
    /// chroma.
    const int MarginX360Luma = 2;

    /// Bit-depth the codec
    /// operates at (input/output files will be converted). (default:
    /// MSBExtendedBitDepth). If different to MSBExtendedBitDepth, source data
    /// will be converted
    const Int InternalBitDepth[MAX_NUM_CHANNEL_TYPE] = {8, 8};
  };

  const static int PredViewID;
  const static int RecoViewID;
  const static int DecodedDebugViewID;

  FrameEncoder(const params &p, const LdpcCoderParameters &LDPC_Params);
  ~FrameEncoder();

  const std::shared_ptr<OutputIcovBase> &icovFile() const;
  void setIcovFile(const std::shared_ptr<OutputIcovBase> &newIcov_file);

  void serializeBlockData(bool access_block,
                          const omnidirectional_layer::Block &b);

protected:
  void encodeBlock(const omnidirectional_layer::Block &b, bool access_block,
                   const omnidirectional_layer::IBlockPredictionManager
                       &block_prediction_mgr) override;

  void encodeBlockWithPrediction(
      const omnidirectional_layer::Block &b,
      omnidirectional_layer::IBlockPredictionManager::prediction_t prediction)
      override;
  void processInput(const IInputMedia<cv::Mat> &input) override;

private:
  void encode_icu(const omnidirectional_layer::Block &b,
                  const icov::video::IcuEncoder &icu_enc,
                  omnidirectional_layer::BlockPrediction::ePredType pred);
  void encode_si(const omnidirectional_layer::Block &b,
                 const icov::video::IcuEncoder &icu_enc,
                 const icov::video::SideInformationUnit &si,
                 omnidirectional_layer::BlockPrediction::ePredType pred,
                 ComponentID comp);
  void encode_tu(const omnidirectional_layer::Block &b,
                 const icov::video::IcuEncoder &icu_enc,
                 const icov::video::SideInformationUnit &si,
                 omnidirectional_layer::BlockPrediction::ePredType pred,
                 ComponentID comp, int tu_index);

  bool debug_enabled() const;

  void serializeSyndrome(bool access_block);

  void serializeParameters();

  void checkDecoding(ldpc_codec_layer::AffInteractiveCoder &encoder,
                     const ldpc_codec_layer::BitSymbolProvider &bsp_enc,
                     const icov::NumcodeIdxVec &si_numcodes,
                     const icov::SyndromesVec &si_synd, ComponentID comp);

  void saveDataToEncode(bool access_block, uint mode,
                        ldpc_codec_layer::AffInteractiveCoder &encoder,
                        const ldpc_codec_layer::BitSymbolProvider &bsp_enc,
                        const icov::NumcodeIdxVec &si_numcodes,
                        const icov::SyndromesVec &si_synd,
                        omnidirectional_layer::BlockPrediction::ePredType pred,
                        ComponentID comp);
  void
  saveDataToDebug(omnidirectional_layer::Block b,
                  ldpc_codec_layer::AffInteractiveCoder &encoder,
                  const ldpc_codec_layer::BitSymbolProvider &bsp_enc,
                  const icov::NumcodeIdxVec &si_numcodes,
                  const icov::SyndromesVec &si_synd,
                  icov::omnidirectional_layer::BlockPrediction::ePredType pred,
                  ComponentID comp);

protected:
  params m_params;
  uint m_numComp;
  icov::video::EncodingStructure m_enc_struc;
  icov::video::BufGroups m_vvc_bufs;
  PredictionsEncoder m_pred_encoder;
  bool m_init;

  std::shared_ptr<OutputIcovBase> m_icov_file;

  int block_comp;

  std::map<size_t, std::vector<uint64_t>> m_rates_histogram;

  cv::Ptr<cv::plot::Plot2d> plot;
  cv::Mat xData, yData, display;

  struct StoredRate {
    NumcodeIdx c; // rate quantized
    Rate r;       // actual rate value
    uint m;       // code length

    //    StoredRate(const ldpc_codec_layer::IInteractiveCoder &enc, NumcodeIdx
    //    num) {

    //    }
  };

  // bitplane coding
  typedef struct {

    //    icov::Syndrome accessBlock;
    icov::Syndrome stored_synd;
    NumcodeIdx stored_synd_rate;
    icov::Rate noSourceThreshold;
    bool sourceFlag;

    // one numcode per SI and per bitplane
    std::map<omnidirectional_layer::IBlockPredictionManager::prediction_t,
             StoredRate>
        stored_numcodes;

  } BitplaneCoder;

  typedef struct {
    ldpc_codec_layer::QaryDistribution stored_pz;
    // mode
    uint dirmode;
  } SiCoder;

  // channel coding
  typedef struct {
    int n;
    std::vector<BitplaneCoder> bitplanes;
    std::vector<BitVec> access_block;
    std::map<omnidirectional_layer::IBlockPredictionManager::prediction_t,
             SiCoder>
        si;
  } ChannelCoder;

  std::vector<ChannelCoder> m_channelCoder;
};

} // namespace encoder
} // namespace icov
