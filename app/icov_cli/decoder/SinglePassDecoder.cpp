#include "SinglePassDecoder.h"
#include "Unit.h"
#include "decoder/BlockDecoder.h"
#include "extractor/BlockExtractor.h"
#include "helpers.h"
#include "omnidirectional_layer/factory.h"
#include "omnidirectional_layer/geometry.h"
#include "omnidirectional_layer/implementation/DummyViewportManager.h"
#include "video_layer/ExtractedStreamDebugger.h"
#include "video_layer/extracted_stream.h"
#include <algorithm>
#include <iterator>
#include <memory>

icov::decoder::SinglePassDecoder::SinglePassDecoder(const fs::path &path)
    : m_icovFile(std::make_shared<InputIcov>(path)),
      m_extractionBitstream(
          std::make_shared<bitstream::IcovExtractedBitstream>()) {}

icov::decoder::SinglePassDecoder::~SinglePassDecoder() {}

void decoder::SinglePassDecoder::initialize(
    const LdpcCoderParameters &coder_params) {

  VideoStreamDebugger::get_mutable_instance().reset();

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

  const point2d resolution(cps.viewport_width, cps.viewport_height);
  const angle_radian fov(cps.viewport_fov_x, cps.viewport_fov_y);

  viewport_geometry vpg = factory::default_geometry(fov, resolution);

  m_navigation.init(
      {

          .VisualizationParams =
              {

                  .Width = icovFile().contentParameterSet().content_width,

                  .Height = icovFile().contentParameterSet().content_height,

                  .ViewportPadding =
                      icovFile().contentParameterSet().viewport_padding

              },

          .IcuSize = icovFile().contentParameterSet().block_fixed_size

      },
      vpg);

  int block_count = icovFile().contentParameterSet().icu_count;
  m_icovFile->readBlockPos(block_count);

  const int channelCount = getNumberValidComponents(chFrmt);
  BlockExtractor::params extractor_params;

  for (int c = 0; c < channelCount; c++) {
    const auto compID = static_cast<ComponentID>(c);
    extractor_params.icu_sizes.push_back(blockArea.block(compID).area());
  }

  m_blockProcessor = std::make_shared<BlockProcessor>();
  m_blockProcessor->blockCount = m_icovFile->contentParameterSet().icu_count;
  m_blockProcessor->ExtractionBitstream = m_extractionBitstream;
  m_blockProcessor->Extractor = std::make_unique<BlockExtractor>(
      m_icovFile, extractor_params, coder_params, m_extractionBitstream);

  m_blockProcessor->Decoder = std::make_unique<BlockDecoder>(
      BlockDecoder::params{

          .qp = cps.qp,

          .chFormat = chFrmt,

          .sourceSize =
              {

                  (int)cps.content_width,

                  (int)cps.content_height

              },

          .icuSize_luma =
              {

                  (int)cps.block_fixed_size,

                  (int)cps.block_fixed_size

              }

      },
      coder_params, m_extractionBitstream);

  m_navigation.Request->setBlockProcessor(m_blockProcessor);
}

const InputIcov &decoder::SinglePassDecoder::icovFile() const {
  return *m_icovFile;
}

const factory::Navigation360 &
decoder::SinglePassDecoder::icovNavigation() const {
  return m_navigation;
}

IBlockProcessor &decoder::SinglePassDecoder::blockProcessor() {
  return *m_blockProcessor;
}

const PelUnitBuf decoder::SinglePassDecoder::getOutputBuf() const {
  return m_blockProcessor->Decoder->decodingOutput();
}

void decoder::SinglePassDecoder::decodeFullFrame() {
  m_navigation.Request->setViewport(std::make_shared<DummyViewportManager>());
  m_navigation.Request->update(0, 0);
}
