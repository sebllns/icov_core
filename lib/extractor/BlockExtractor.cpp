#include "BlockExtractor.h"
#include "extractor/ChannelExtractor.h"
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

icov::extractor::BlockExtractor::BlockExtractor(
    std::shared_ptr<InputIcov> &icovFile, const params &p,
    const LdpcCoderParameters &coder_params,
    const bitstream::sp_IcovExtractedBitstream &extraction)
    : m_icovFile(icovFile), m_extractionBitstream(extraction),
      m_channelCount(p.icu_sizes.size()), m_params(p) {

  bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingFrame(0);

  bitstream::int8b compID = 0;
  for (int n : p.icu_sizes) {
    auto channel_params = coder_params.clone();
    channel_params->codeLength = n;
    ChannelExtractor::params channel_extractor_params{.CompID = compID};
    m_channelExtractor.push_back(std::make_unique<ChannelExtractor>(
        channel_extractor_params, m_icovFile, m_extractionBitstream,
        *channel_params));
    compID++;
  }
}

icov::extractor::BlockExtractor::~BlockExtractor() {}

void icov::extractor::BlockExtractor::startProcess() {}

void icov::extractor::BlockExtractor::processBlock(
    const Block &b,
    icov::omnidirectional_layer::IBlockPredictionManager::prediction_t
        prediction) {

  icov::omnidirectional_layer::BlockPrediction::predType block_prediction =
      icov::omnidirectional_layer::BlockPrediction::toPredType(prediction);
  bool is_access_block =
      (block_prediction ==
       icov::omnidirectional_layer::BlockPrediction::predType::NONE);

  std::cout << "SELECT BLOCK " << b.getIdSelf() << std::endl;
  m_icovFile->selectBlock(b.getIdSelf() - 1);

  // get stored syndrome size per bitplane
  std::cout << "extract_syndrome_sizes" << std::endl;
  for (auto &ch : m_channelExtractor) {
    ch->extract_syndrome_sizes(is_access_block);
  }

  if (!is_access_block) {
    // pass access block syndrome  per bitplane
    if (b.isAccessBlock()) {
      std::cout << "PASS ACCESS BLOCK" << std::endl;
      ChannelExtractor::resetSyndromePadding();
      for (auto &ch : m_channelExtractor) {
        ch->pass_access_block();
      }
    }

    // select prediction
    std::cout << "SELECT PREDICTION" << std::endl;
    int pred_idx = bitstream::getPredictionOrder(block_prediction);
    for (int i = 0; i < pred_idx; ++i) {
      for (auto &ch : m_channelExtractor)
        ch->pass_prediction();
    }
  }

  ChannelExtractor::resetSyndromePadding();
  std::cout << "extract_block" << std::endl;
  for (auto &ch : m_channelExtractor) {
    ch->extract_block(b, block_prediction);
  }

  //  if (is_access_block && ChannelExtractor::getSyndromePadding() > 0)
  //    m_extractionBitstream->back() = ((m_extractionBitstream->back() >>
  //                                      ChannelExtractor::getSyndromePadding())
  //                                     <<
  //                                     ChannelExtractor::getSyndromePadding());
  // m_icovFile
  // BlockPrediction::predType::NONE
}

void icov::extractor::BlockExtractor::endProcess() {}
