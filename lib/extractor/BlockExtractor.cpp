#include "BlockExtractor.h"
#include "BlockPrediction.h"

icov::extractor::BlockExtractor::BlockExtractor(
    std::shared_ptr<InputIcov> &icovFile, std::shared_ptr<std::ostream> &stream,
    const params &p)
    : m_icovFile(icovFile), m_stream(stream), m_params(p) {}

icov::extractor::BlockExtractor::~BlockExtractor() {}

void icov::extractor::BlockExtractor::startProcess() {}

void icov::extractor::BlockExtractor::processBlock(const Block &b,
                                                   uint16_t prediction) {

  // m_icovFile
  // BlockPrediction::predType::NONE
}

void icov::extractor::BlockExtractor::endProcess() {}
