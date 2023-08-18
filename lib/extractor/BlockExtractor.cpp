#include "BlockExtractor.h"
#include "extractor/ChannelExtractor.h"
#include "omnidirectional_layer/interface/IBlockProcessor.h"
#include "video_layer/ExtractedStreamDebugger.h"
#include "video_layer/encoded_stream.h"
#include "video_layer/extracted_stream.h"
#include "video_layer/icov_stream.h"
#include <algorithm>
#include <iterator>
#include <memory>

using namespace icov;
using namespace icov::extractor;
using namespace icov::omnidirectional_layer;
using namespace icov::bitstream;

icov::extractor::BlockExtractor::BlockExtractor(std::shared_ptr<InputIcov> &icovFile, const params &p,
                                                const LdpcCoderParameters &coder_params,
                                                const bitstream::sp_IcovExtractedBitstream &extraction)
    : omnidirectional_layer::IBlockProcessor(), m_icovFile(icovFile), m_extractionBitstream(extraction),
      m_channelCount(p.icu_sizes.size()), m_params(p)
{
    this->setName("Extractor");

    bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingFrameID(0);

    bitstream::int8b compID = 0;
    for (int n : p.icu_sizes)
    {
        auto channel_params = coder_params.clone();
        channel_params->codeLength = n;
        ChannelExtractor::params channel_extractor_params{.CompID = compID};
        m_channelExtractor.push_back(std::make_unique<ChannelExtractor>(channel_extractor_params, m_icovFile,
                                                                        m_extractionBitstream, *channel_params));
        compID++;
    }
}

icov::extractor::BlockExtractor::~BlockExtractor()
{
}

void icov::extractor::BlockExtractor::_processBlock(
    const request_t &r, const Block &b, icov::omnidirectional_layer::IBlockPredictionManager::prediction_t prediction)
{

    icov::omnidirectional_layer::BlockPrediction::ePredType block_prediction =
        icov::omnidirectional_layer::BlockPrediction::toPredType(prediction);

    ICOV_DEBUG << "START EXTRACT BLOCK " << b.getIdSelf() << " WITH PREDICTION "
               << omnidirectional_layer::BlockPrediction::toString(block_prediction);

    bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingBlockID(b.getIdSelf() - 1);
    bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingPredictionID(block_prediction);

    bool is_access_block = (block_prediction == icov::omnidirectional_layer::BlockPrediction::ePredType::NONE);

    std::cout << "SELECT BLOCK " << b.getIdSelf() << std::endl;
    m_icovFile->selectBlock(b.getIdSelf());

    // get stored syndrome size per bitplane
    std::cout << "extract_syndrome_sizes" << std::endl;
    for (auto &ch : m_channelExtractor)
    {
        ch->extract_stored_syndrome_size(is_access_block);
    }

    if (!is_access_block)
    {
        // pass access block syndrome  per bitplane
        if (b.isAccessBlock())
        {
            ICOV_TRACE << "PASS ACCESS BLOCK";
            ChannelExtractor::resetSyndromePadding();
            for (auto &ch : m_channelExtractor)
            {
                ch->pass_access_block();
            }
        }

        // select prediction
        ICOV_TRACE << "SELECT PREDICTION";
        int pred_idx = bitstream::getPredictionOrder(block_prediction);
        for (int i = 0; i < pred_idx; ++i)
        {
            for (auto &ch : m_channelExtractor)
                ch->pass_prediction();
        }
    }

    ChannelExtractor::resetSyndromePadding();
    int c = 0;
    for (auto &ch : m_channelExtractor)
    {
        ICOV_TRACE << "START EXTRACT CHANNEL " << c;
        ch->extract_block_syndrome(b, block_prediction);
        ICOV_TRACE << "END EXTRACT CHANNEL " << c;
    }

    //  if (is_access_block && ChannelExtractor::getSyndromePadding() > 0)
    //    m_extractionBitstream->back() = ((m_extractionBitstream->back() >>
    //                                      ChannelExtractor::getSyndromePadding())
    //                                     <<
    //                                     ChannelExtractor::getSyndromePadding());
    // m_icovFile
    // BlockPrediction::predType::NONE

    ICOV_DEBUG << "END EXTRACT BLOCK " << b.getIdSelf() << " WITH PREDICTION "
               << omnidirectional_layer::BlockPrediction::toString(block_prediction);
}

void icov::extractor::BlockExtractor::_beforeOrderingLoop(const request_t &r)
{
}

void icov::extractor::BlockExtractor::_afterOrderingLoop(const request_t &r)
{
}
