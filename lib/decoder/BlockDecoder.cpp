#include "BlockDecoder.h"
#include "decoder/ChannelDecoder.h"

#include "icov_debug.h"
#include "icov_log.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "video_layer/Encodingstructure.h"
#include "video_layer/ExtractedStreamDebugger.h"
#include "video_layer/ICOV_Quant.h"
#include "video_layer/vtm_utility.h"
#include <boost/algorithm/algorithm.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <memory>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

namespace fs = boost::filesystem;
using namespace icov::decoder;
using SI_SIDE = icov::omnidirectional_layer::BlockPrediction::ePredType;

BlockDecoder::BlockDecoder(const params &p, const LdpcCoderParameters &coder_params,
                           const bitstream::sp_IcovExtractedBitstream &extraction)
    : IBlockDecoder(), m_params(p), m_rate(0)
{
    // CONSTANTS
    const Size icuSize_luma = Size(m_params.icuSize_luma.width, m_params.icuSize_luma.height);
    const Size tuSize_luma = Size(m_params.icuSize_luma.width, m_params.icuSize_luma.height);

    const int sourceWidth = (int)p.sourceSize.width;
    const int sourceHeight = (int)p.sourceSize.height;
    const int margin_x_360luma = m_params.margin_x_360luma;

    const UnitArea extendedFrameArea(p.chFormat, Area(0, 0, sourceWidth + 2 * margin_x_360luma, sourceHeight));

    mainFrameArea = UnitArea(p.chFormat, Area(margin_x_360luma, 0, sourceWidth, sourceHeight));

    m_vvc_bufs = std::make_shared<icov::video::BufGroups>();
    m_vvc_bufs->create(PIC_ORIGINAL, extendedFrameArea);
    m_vvc_bufs->create(PIC_RECONSTRUCTION, extendedFrameArea);
    m_vvc_bufs->create(PIC_PREDICTION, extendedFrameArea);

    m_decodingOutput = getDecodingOutput();

    m_dec_struc = std::make_shared<video::EncodingStructure>();

    m_dec_struc->create360VideoInteractiveCodingUnits_withoutAccessBlockComputation(
        m_params.chFormat, m_params.sourceSize.width, m_params.sourceSize.height, m_params.margin_x_360luma,
        icuSize_luma.width, icuSize_luma.height, tuSize_luma.width, tuSize_luma.height);

    const int channelCount = getNumberValidComponents(p.chFormat);

    UnitArea blockArea(p.chFormat, Area(0, 0, p.icuSize_luma.width, p.icuSize_luma.height));

    for (int c = 0; c < channelCount; c++)
    {
        const auto compID = static_cast<ComponentID>(c);
        auto coder_channel_params = coder_params.clone();
        coder_channel_params->codeLength = blockArea.block(compID).area();

        const ChannelType channelType = toChannelType(compID);
        const int bit_depth = p.internalBitDepth[channelType];
        const icov::video::QpParam cQP(p.qp, channelType, p.chFormat, bit_depth, 0, 0);
        ChannelDecoder::params channel_params{

            .QpParam = cQP,

            .CompID = compID,

            .bit_depth = bit_depth,

            .maxLog2TrDynamicRange = p.maxLog2TrDynamicRange

        };

        m_channelDecoder.push_back(std::make_unique<ChannelDecoder>(channel_params, *coder_channel_params, m_vvc_bufs,
                                                                    m_dec_struc, extraction));
    }
}

BlockDecoder::~BlockDecoder() {}

const PelUnitBuf &BlockDecoder::decodingOutput() const {
  return m_decodingOutput;
}

PelUnitBuf BlockDecoder::getDecodingOutput() {
  return m_vvc_bufs->get(PIC_RECONSTRUCTION).getBuf(mainFrameArea);
}

icov::Rate BlockDecoder::rate() const
{
  return m_rate;
}

void icov::decoder::BlockDecoder::_beforeOrderingLoop(const request_t &r)
{
}

void icov::decoder::BlockDecoder::_afterOrderingLoop(const request_t &r)
{
}

void icov::decoder::BlockDecoder::_processBlock(const request_t &r, const omnidirectional_layer::Block &b,
                                                omnidirectional_layer::IBlockPredictionManager::prediction_t prediction)
{
  omnidirectional_layer::BlockPrediction::ePredType pred =
      omnidirectional_layer::BlockPrediction::toPredType(prediction);

  ICOV_DEBUG << "START DECODE BLOCK " << b.getIdSelf() << " WITH PREDICTION "
             << omnidirectional_layer::BlockPrediction::toString(pred);

  icov::assert::check(omnidirectional_layer::BlockPrediction::isValidPrediction(pred), "Bad prediction value");

  int x_block = b.parent()->col(b.u());
  int y_block = b.parent()->row(b.v());

  Position pos(static_cast<float>(x_block) / static_cast<float>(m_params.icuSize_luma.width),
               static_cast<float>(y_block) / static_cast<float>(m_params.icuSize_luma.height));

  std::cout << pos.x << std::endl;
  std::cout << pos.y << std::endl;

  const auto &es = *this->m_dec_struc;
  uint icuIndex = es.get_icuIndex(pos);

  std::cout << icuIndex << std::endl;

  bitstream::VideoStreamDebugger::get_mutable_instance().reset();

  bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingBlockID(b.getIdSelf() - 1);
  bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingPredictionID(pred);

  m_rate = 0;
  for (auto &dec : m_channelDecoder)
  {
        if (icov::debug::debug_trace_enabled)
            ICOV_TRACE << "START DECODE COMP " << dec->get_params().CompID;

        dec->decode_icu(icuIndex, pred);
        m_rate += dec->rate();

        //@todo TODO: copy only necessary pixels
        VTM_Utility::copyBordersFor360(m_decodingOutput, dec->get_params().CompID);

        if (icov::debug::debug_trace_enabled)
            ICOV_TRACE << "END DECODE COMP " << dec->get_params().CompID;
  }

  m_rate /= m_channelDecoder.size();

  ICOV_DEBUG << "END DECODE BLOCK " << b.getIdSelf() << " WITH PREDICTION "
             << omnidirectional_layer::BlockPrediction::toString(pred);
}
