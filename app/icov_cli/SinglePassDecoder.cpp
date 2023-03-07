#include "SinglePassDecoder.h"
#include "BlockPrediction.h"
#include "Unit.h"
#include "encoded_stream.h"
#include "extracted_stream.h"
#include "extractor/BlockExtractor.h"
#include "extractor/ChannelExtractor.h"
#include "helpers.h"
#include <algorithm>
#include <iterator>
#include <memory>

icov::decoder::SinglePassDecoder::SinglePassDecoder(const fs::path &path)
    : m_icovFile(std::make_shared<InputIcov>(path)),
      m_extractionBitstream(std::make_shared<IcovExtractedBitstream>()) {}

icov::decoder::SinglePassDecoder::~SinglePassDecoder() {}

void decoder::SinglePassDecoder::initialize(
    const CoderParameters &coder_params) {
  ICOV_DEBUG << "INIT SinglePassDecoder";
  m_icovFile->readContentParameterSet();
  const auto cps = m_icovFile->contentParameterSet();

  ICOV_DEBUG << "Icov version: " << cps.icov_version;
  ICOV_DEBUG << (int)cps.content_chroma_format;
  ICOV_DEBUG << (int)cps.content_height;
  ICOV_DEBUG << (int)cps.content_width;
  ICOV_DEBUG << (int)cps.block_fixed_size;
  ICOV_DEBUG << (int)cps.qp;

  const ChromaFormat chFrmt = (ChromaFormat)cps.content_chroma_format;

  const int icu_size = (int)cps.block_fixed_size;
  UnitArea blockArea(chFrmt, Area(0, 0, icu_size, icu_size));

  icov::helpers::assert::check(
      cps.is_block_fixed_size,
      "Only fixed size blocks is available in this version");

  const int channelCount = getNumberValidComponents(chFrmt);
  BlockExtractor::params extractor_params;

  for (int c = 0; c < channelCount; c++) {
    const auto compID = static_cast<ComponentID>(c);
    extractor_params.icu_sizes.push_back(blockArea.block(compID).area());
  }
  m_extractor = std::make_unique<BlockExtractor>(
      m_icovFile, extractor_params, coder_params, m_extractionBitstream);
}

void icov::decoder::SinglePassDecoder::startProcess() {
  if (m_extractor)
    m_extractor->startProcess();
  if (m_decoder)
    m_decoder->startProcess();
}

void icov::decoder::SinglePassDecoder::processBlock(const Block &b,
                                                    uint16_t prediction) {
  if (m_extractor)
    m_extractor->processBlock(b, prediction);
  if (m_decoder)
    m_decoder->processBlock(b, prediction);
}

void icov::decoder::SinglePassDecoder::endProcess() {
  if (m_extractor)
    m_extractor->endProcess();
  if (m_decoder)
    m_decoder->endProcess();
}

const InputIcov &icov::decoder::SinglePassDecoder::icovFile() const {
  return *m_icovFile;
}
