#include "BlockExtractor.h"
#include "BlockPrediction.h"
#include "extractor/ChannelExtractor.h"
#include "helpers.h"
#include <algorithm>
#include <iterator>

icov::extractor::BlockExtractor::BlockExtractor(
    std::shared_ptr<InputIcov> &icovFile, byte *syndrome, const params &p,
    const CoderParameters &coder_params)
    : m_icovFile(icovFile), m_syndrome_buf(syndrome), m_params(p) {

  icov::helpers::assert::not_null(m_syndrome_buf,
                                  "Syndrome buffer not allocated");

  for (int n : p.icu_sizes) {
    auto channel_params = coder_params.clone();
    channel_params->n = n;
    m_channelExtractor.push_back(ChannelExtractor(m_icovFile, *channel_params));
  }
}

icov::extractor::BlockExtractor::~BlockExtractor() {}

void icov::extractor::BlockExtractor::startProcess() {}

void icov::extractor::BlockExtractor::processBlock(const Block &b,
                                                   uint16_t prediction) {

  BlockPrediction::predType block_prediction =
      BlockPrediction::toPredType(prediction);

  m_icovFile->selectBlock(b.getIdSelf() - 1);

  if (block_prediction == BlockPrediction::predType::NONE) {
    // access block
  } else {
    if (b.isAccessBlock()) {
      // pass access block syndrome
    }
  }

  // m_icovFile
  // BlockPrediction::predType::NONE
}

void icov::extractor::BlockExtractor::endProcess() {}
