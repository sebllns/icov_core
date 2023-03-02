#include "BlockExtractor.h"
#include "BlockPrediction.h"
#include "encoded_stream.h"
#include "extracted_stream.h"
#include "extractor/ChannelExtractor.h"
#include "helpers.h"
#include <algorithm>
#include <iterator>
#include <memory>

icov::extractor::BlockExtractor::BlockExtractor(
    std::shared_ptr<InputIcov> &icovFile, byte *syndrome, const params &p,
    const CoderParameters &coder_params,
    const sp_IcovExtractedBitstream &extraction)
    : m_icovFile(icovFile), m_channelCount(p.icu_sizes.size()),
      /*m_bitplaneCount(m_channelCount),*/ m_syndrome_buf(syndrome),
      m_params(p), m_extractionBitstream(extraction) {

  icov::helpers::assert::not_null(m_syndrome_buf,
                                  "Syndrome buffer not allocated");

  for (int n : p.icu_sizes) {
    auto channel_params = coder_params.clone();
    channel_params->n = n;
    m_channelExtractor.push_back(std::make_unique<ChannelExtractor>(
        m_icovFile, m_extractionBitstream, *channel_params));
  }
}

icov::extractor::BlockExtractor::~BlockExtractor() {}

void icov::extractor::BlockExtractor::startProcess() {}

void icov::extractor::BlockExtractor::processBlock(const Block &b,
                                                   uint16_t prediction) {

  BlockPrediction::predType block_prediction =
      BlockPrediction::toPredType(prediction);
  bool is_access_block = (block_prediction == BlockPrediction::predType::NONE);

  m_icovFile->selectBlock(b.getIdSelf() - 1);

  // get stored syndrome size per bitplane
  for (auto &ch : m_channelExtractor) {
    ch->extract_syndrome_sizes(is_access_block);
  }

  if (!is_access_block) {
    // pass access block syndrome  per bitplane
    if (b.isAccessBlock()) {
      ChannelExtractor::resetSyndromePadding();
      for (auto &ch : m_channelExtractor) {
        ch->pass_access_block();
      }
    }

    // select prediction
    int pred_idx = getPredictionOrder(block_prediction);
    for (int i = 0; i < pred_idx; ++i) {
      for (auto &ch : m_channelExtractor)
        ch->pass_prediction();
    }
  }

  ChannelExtractor::resetSyndromePadding();
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
