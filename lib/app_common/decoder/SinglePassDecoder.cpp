#include "SinglePassDecoder.h"
#include "video_layer/ips.h"

#include "Media.h"
#include "Unit.h"
#include "decoder/BlockDecoder.h"
#include "extractor/BlockExtractor.h"
#include "icov_assert.h"
#include "icov_log.h"
#include "icov_math.h"
#include "icov_request.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/factory.h"
#include "omnidirectional_layer/icov_geometry.h"
#include "omnidirectional_layer/interface/IBlockProcessor.h"
#include "video_layer/ExtractedStreamDebugger.h"
#include "video_layer/extracted_stream.h"
#include "video_layer/video_common.h"

#include <boost/filesystem/operations.hpp>
#include <boost/format.hpp>

#include <algorithm>
#include <cstdio>
#include <exception>
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <vector>

std::unique_ptr<LdpcCoderParameters>
parseLdpcParams(const ContentParameterSet &cps) {
  std::string coder_type_val = cps.ldpc_coder_type;
  std::unique_ptr<LdpcCoderParameters> params =
      ldpc_codec_layer::createLdpcParams(coder_type_val);
  icov::assert::not_null(params.get());
  params->codeLength = cps.block_fixed_size * cps.block_fixed_size;
  params->beliefPropagationIterations = cps.ldpc_bp_ite;
  return params;
}

icov::decoder::SinglePassDecoder::SinglePassDecoder()
    : m_extractionBitstream(
          std::make_shared<bitstream::IcovExtractedBitstream>()),
      m_blockProcessor(std::make_shared<IBlockProcessor>()),
      m_request(request_t::undefined()) {
  m_blockProcessor->setName("Root");
  m_blockProcessor->setEnabled(true);
  m_gops = 0;
  m_current_gop = 0;
  m_gop_frame = 0;
}

icov::decoder::SinglePassDecoder::~SinglePassDecoder() {}

void decoder::SinglePassDecoder::initLocal(const params &p) {
  ICOV_INFO << "Init local SinglePassDecoder";

  setParams(p);

  ICOV_INFO << "------------------";
  ICOV_INFO << "| Open ICOV file |";
  ICOV_INFO << "------------------";
  openIcovFile(p.IcovFile);

  const auto &cps = m_contentParams;
  const auto &media_type = static_cast<eMediaType>(cps.content_type);

  ICOV_INFO << boost::format("%-30s %-s") % "Icov version:" % cps.icov_version;
  ICOV_INFO << boost::format("%-30s %-s") % "Type:" % mediaTypeStr(media_type);
  ICOV_INFO << boost::format("%-30s %-s") % "Chroma subsampling:" %
                   video::chromaSubsamplingToStr(
                       (int)cps.content_chroma_format);

  ICOV_INFO << boost::format("%-30s %-d") % "ERP Width:" %
                   (int)cps.content_width;
  ICOV_INFO << boost::format("%-30s %-d") % "ERP Height:" %
                   (int)cps.content_height;

  ICOV_INFO << boost::format("%-30s %-d") % "VP Width:" %
                   (int)cps.viewport_width;
  ICOV_INFO << boost::format("%-30s %-d") % "VP Height:" %
                   (int)cps.viewport_height;
  ICOV_INFO << boost::format("%-30s %-.2f") % "VP H-FOV (DEG):" %
                   ((float)cps.viewport_fov_x * RAD2DEG);
  ICOV_INFO << boost::format("%-30s %-.2f") % "VP V-FOV (DEG):" %
                   ((float)cps.viewport_fov_y * RAD2DEG);
  ICOV_INFO << boost::format("%-30s %-.3f") % "VP Padding:" %
                   cps.viewport_padding;
  ICOV_INFO << boost::format("%-30s %-d") % "Block size:" %
                   (int)cps.block_fixed_size;
  ICOV_INFO << boost::format("%-30s %-d") % "QP:" % (int)cps.qp;

  auto coder_params = parseLdpcParams(cps);
  coder_params->database = p.LdpcDatabase.string();
  setLdpcParams(*coder_params);

  ICOV_INFO << "Init 360 navigation";
  addDefaultNavigation();

  ICOV_INFO << "Init extractor";
  addDefaultExtractor();

  ICOV_INFO << "Init decoder";
  addDefaultDecoder();

  IBlockProcessor::func_t check_empty_bitsteam =
      [this](const request_t &, const Block &,
             IBlockPredictionManager::prediction_t) {
        ICOV_DEBUG << "BUFFER SIZE = "
                   << m_extractionBitstream->getActualSize();
        assert::check(
            m_extractionBitstream->isEmpty(),
            ICOV_DEBUG_HEADER("Bitstream must be empty after block decoding"));
      };
  m_blockDecoder->setAfter(check_empty_bitsteam);
  m_request = request_t::undefined();
  m_video_frame = 0;
}

void decoder::SinglePassDecoder::openIcovFile(
    const boost::filesystem::path &path) {

  ICOV_INFO << boost::format("%-30s %-d") % "Open ICOV file:" % path;

  m_icovFile = std::make_shared<InputIcov>(path);

  // the client side only gets the parameter set bytes, as over a network
  setContentParams(bitstream::read_ips(m_icovFile->parameterSetBytes()));

  const auto &cps = m_contentParams;
  const auto &media_type = static_cast<eMediaType>(cps.content_type);

  if (media_type == eMediaType::VIDEO) {
    ICOV_INFO << boost::format("%-30s %-d") % "Framecount:" %
                     (int)cps.content_framecount;
    ICOV_INFO << boost::format("%-30s %-.3f") % "Framerate:" %
                     (int)cps.content_framerate;
  }
}

void decoder::SinglePassDecoder::addDefaultNavigation() {
  m_navigation = std::make_shared<factory::Navigation360>();

  const auto &cps = m_contentParams;

  const point2d resolution(cps.viewport_width, cps.viewport_height);
  const angle_radian fov(cps.viewport_fov_x, cps.viewport_fov_y);

  viewport_geometry vpg = factory::default_geometry(fov, resolution);

  std::vector<Block::id_t> visibles;
  std::transform(cps.encoded_blocks.cbegin(), cps.encoded_blocks.cend(),
                 std::back_inserter(visibles),
                 [](const auto &pair) { return pair.first; });

  m_navigation->init(
      {

          .VisualizationParams =
              {

                  .Width = cps.content_width,

                  .Height = cps.content_height,

                  .DummyViewport = m_params.DummyViewport,

                  .ViewportPadding = cps.viewport_padding

              },

          .IcuSize = cps.block_fixed_size,

          .VisibleBlocks = visibles

      },
      vpg);

  m_navigation->Request->setBlockProcessor(m_blockProcessor);
}

void decoder::SinglePassDecoder::addDefaultExtractor() {
  const auto &cps = m_contentParams;
  const ChromaFormat chFrmt =
      static_cast<ChromaFormat>(cps.content_chroma_format);

  const int channelCount = getNumberValidComponents(chFrmt);
  BlockExtractor::params extractor_params;

  const int icu_size = (int)cps.block_fixed_size;
  UnitArea blockArea(chFrmt, Area(0, 0, icu_size, icu_size));

  for (int c = 0; c < channelCount; c++) {
    const auto compID = static_cast<ComponentID>(c);
    extractor_params.icu_sizes.push_back(blockArea.block(compID).area());
  }

  setBlockExtractor(std::make_shared<BlockExtractor>(
      m_icovFile, extractor_params, ldpcParams(), m_extractionBitstream));
}

void decoder::SinglePassDecoder::addDefaultDecoder() {

  const ContentParameterSet &cps = m_contentParams;

  setBlockDecoder(std::make_shared<BlockDecoder>(
      BlockDecoder::params{

          .qp = cps.qp,

          .chFormat = static_cast<ChromaFormat>(cps.content_chroma_format),

          .sourceSize =
              {

                  (int)cps.content_width,

                  (int)cps.content_height

              },

          .icuSize_luma =
              {

                  (int)cps.block_fixed_size,

                  (int)cps.block_fixed_size

              },
          .bypass_decoding = false

      },
      ldpcParams(), m_extractionBitstream));
}

void decoder::SinglePassDecoder::resetCache() {
  m_navigation->Request->resetCache();
}

const PelUnitBuf decoder::SinglePassDecoder::getDecodedBuf() const {
  icov::assert::not_null(m_blockDecoder.get(),
                         ICOV_DEBUG_HEADER("Decoder cannot be null"));
  return m_blockDecoder->decodingOutput();
}

void decoder::SinglePassDecoder::start() {
  if (m_blockDecoder)
    m_blockDecoder->getDecodingOutput().fill(0);
  m_navigation->Request->start();

  //  frame = 0;
  // this->request(0, 0, 0);
}

void icov::decoder::SinglePassDecoder::catchError() {
  ICOV_ERROR << "Error decoding the frame " << m_request.frameID;
  m_icovFile->resetStreams();
  m_extractionBitstream->empty();
}

bool decoder::SinglePassDecoder::setRequest(const request_t &r) {
  //  if (m_blockDecoder)
  //    m_blockDecoder->getDecodingOutput().fill(0);

  bool decoded = false;
  icov::assert::not_null(m_navigation.get(),
                         ICOV_DEBUG_HEADER("Navigation cannot be null"));
  icov::assert::not_null(m_navigation->Request.get(),
                         ICOV_DEBUG_HEADER("Request manager cannot be null"));

  VideoStreamDebugger::get_mutable_instance().reset();

  int cg = m_current_gop;
  int cf = m_gop_frame;

  if (m_request != r) {
    try {

      int delta_frame = 0;

      if (m_request.frameID != m_request.undefined().frameID) {
        delta_frame = r.frameID - m_video_frame;
      }

      m_request = r;
      m_video_frame = r.frameID;

      request_t &tmp_r = m_request;

      bool play = false;

      if (m_gops > 0) {
        tmp_r.frameID = m_gop_frame;
        for (int f = 0; f < delta_frame; ++f) {
          ++m_gop_frame;
          if (m_gop_frame >= m_contentParams.content_framecount) {
            ++m_current_gop;
            m_gop_frame = 0;
            if (getParams().LoopMode) {
              play = true;
              m_current_gop %= m_gops;
            }

            if (m_current_gop < m_gops) {
              m_params.IcovFile = m_icovFile->getPath().parent_path() /
                                  (std::to_string(m_current_gop) + ".icov");
              if (fs::is_regular_file(m_params.IcovFile)) {
                this->openIcovFile(m_params.IcovFile);
                if (m_blockExtractor)
                  m_blockExtractor->setIcovFile(m_icovFile);
              }
            } else {
              play = false;
            }
          }
        }

        if (m_current_gop < m_gops &&
            m_gop_frame < m_contentParams.content_framecount) {
          play = true;
          tmp_r.frameID = m_gop_frame;
        }

        if (m_current_gop == cg && m_gop_frame == cf)
          play = false;

      } else if (tmp_r.frameID < m_contentParams.content_framecount) {
        play = true;
      } else if (getParams().LoopMode) {
        play = true;
        tmp_r.frameID %= m_contentParams.content_framecount;
        m_request.frameID = tmp_r.frameID;
      }

      m_extractionBitstream->empty();

      if (play) {
        ICOV_INFO << "GOP " << m_current_gop << " Request " << to_string(tmp_r);
        m_navigation->Request->update(tmp_r);
        decoded = true;
      }
    } catch (const std::exception &e) {
      ICOV_ERROR << e.what();
      catchError();
    } catch (...) {
      catchError();
    }
  }

  return decoded;
}

const request_t &decoder::SinglePassDecoder::getRequest() const {
  return m_request;
}

const LdpcCoderParameters &decoder::SinglePassDecoder::ldpcParams() const {
  assert::not_null(m_ldpcParams.get(),
                   ICOV_DEBUG_HEADER("LDPC parameters cannot be null"));
  return *m_ldpcParams;
}

void decoder::SinglePassDecoder::setLdpcParams(
    const LdpcCoderParameters &newLdpcParams) {
  m_ldpcParams = newLdpcParams.clone();
}

const ContentParameterSet &decoder::SinglePassDecoder::contentParams() const {
  return m_contentParams;
}

const std::shared_ptr<factory::Navigation360> &
decoder::SinglePassDecoder::navigation() const {
  return m_navigation;
}

void decoder::SinglePassDecoder::setNavigation(
    const std::shared_ptr<factory::Navigation360> &newNavigation) {
  m_navigation = newNavigation;
}

const std::shared_ptr<InputIcov> &decoder::SinglePassDecoder::icovFile() const {
  return m_icovFile;
}

void decoder::SinglePassDecoder::setIcovFile(
    const std::shared_ptr<InputIcov> &newIcovFile) {
  m_icovFile = newIcovFile;
}

const sp_IcovExtractedBitstream &
decoder::SinglePassDecoder::extractionBitstream() const {
  return m_extractionBitstream;
}

void decoder::SinglePassDecoder::setContentParams(
    const ContentParameterSet &newContentParams) {
  m_contentParams = newContentParams;
  m_contentParams.content_framerate = 30;
}

const decoder::SinglePassDecoder::params &
decoder::SinglePassDecoder::getParams() const {
  return m_params;
}

void decoder::SinglePassDecoder::setParams(
    const decoder::SinglePassDecoder::params &newParams) {
  m_params = newParams;
}

void decoder::SinglePassDecoder::setGopCount(int c) { m_gops = c; }

const std::shared_ptr<BlockExtractor> &
decoder::SinglePassDecoder::blockExtractor() const {
  return m_blockExtractor;
}

const std::shared_ptr<decoder::BlockDecoder> &
decoder::SinglePassDecoder::blockDecoder() const {
  return m_blockDecoder;
}

const std::shared_ptr<IBlockProcessor> &
decoder::SinglePassDecoder::blockProcessor() const {
  return m_blockProcessor;
}

void decoder::SinglePassDecoder::setBlockExtractor(
    const std::shared_ptr<BlockExtractor> &newBlockExtractor) {
  m_blockExtractor = newBlockExtractor;
  IBlockProcessor::addChild(m_blockProcessor, m_blockExtractor);
}

void decoder::SinglePassDecoder::setBlockDecoder(
    const std::shared_ptr<decoder::BlockDecoder> &newBlockDecoder) {
  m_blockDecoder = newBlockDecoder;
  IBlockProcessor::addChild(m_blockProcessor, m_blockDecoder);
}
