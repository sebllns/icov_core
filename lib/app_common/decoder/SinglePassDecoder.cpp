#include "SinglePassDecoder.h"
#include "Media.h"
#include "Unit.h"
#include "decoder/BlockDecoder.h"
#include "extractor/BlockExtractor.h"

#include "icov_assert.h"
#include "icov_log.h"
#include "icov_request.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/factory.h"
#include "omnidirectional_layer/icov_geometry.h"
#include "omnidirectional_layer/implementation/DummyViewportManager.h"
#include "omnidirectional_layer/interface/IBlockProcessor.h"
#include "video_layer/ExtractedStreamDebugger.h"
#include "video_layer/extracted_stream.h"
#include "video_layer/video_common.h"
#include <algorithm>
#include <boost/format.hpp>
#include <iterator>
#include <limits>
#include <memory>
#include <vector>

std::unique_ptr<LdpcCoderParameters> parseLdpcParams(const ContentParameterSet &cps)
{
    std::string coder_type_val = cps.ldpc_coder_type;
    std::unique_ptr<LdpcCoderParameters> params = ldpc_codec_layer::createLdpcParams(coder_type_val);
    icov::assert::not_null(params.get());
    params->codeLength = cps.block_fixed_size * cps.block_fixed_size;
    params->beliefPropagationIterations = cps.ldpc_bp_ite;
    return params;
}

icov::decoder::SinglePassDecoder::SinglePassDecoder()
    : m_extractionBitstream(std::make_shared<bitstream::IcovExtractedBitstream>()),
      m_blockProcessor(std::make_shared<IBlockProcessor>()), pov_x(std::numeric_limits<float>::max()), pov_y(pov_x),
      frame(std::numeric_limits<unsigned long>::max())
{
    m_blockProcessor->setName("Root");
    m_blockProcessor->setEnabled(true);
}

icov::decoder::SinglePassDecoder::~SinglePassDecoder() {}

void decoder::SinglePassDecoder::initLocal(const params &p)
{
    ICOV_INFO << "Init local SinglePassDecoder";

    setParams(p);

    ICOV_INFO << "Open ICOV file";
    openIcovFile(p.IcovFile);

    const auto &cps = m_contentParams;

    ICOV_INFO << boost::format("%-30s %-s") % "Icov version:" % cps.icov_version;
    ICOV_INFO << boost::format("%-30s %-s") % "Type:" % mediaTypeStr(static_cast<eMediaType>(cps.content_type));
    ICOV_INFO << boost::format("%-30s %-s") % "Chroma subsampling:" %
                     video::chromaSubsamplingToStr((int)cps.content_chroma_format);

    ICOV_INFO << boost::format("%-30s %-d") % "Width:" % (int)cps.content_width;
    ICOV_INFO << boost::format("%-30s %-d") % "Height:" % (int)cps.content_height;

    ICOV_INFO << (int)cps.block_fixed_size;
    ICOV_INFO << (int)cps.qp;

    auto coder_params = parseLdpcParams(cps);
    coder_params->database = p.LdpcDatabase.string();
    setLdpcParams(*coder_params);

    ICOV_INFO << "Init 360 navigation";
    addDefaultNavigation();

    ICOV_INFO << "Init extractor";
    addDefaultExtractor();

    ICOV_INFO << "Init decoder";
    addDefaultDecoder();

    IBlockProcessor::func_t check_empty_bitsteam = [this](const request_t &, const Block &,
                                                          IBlockPredictionManager::prediction_t) {
        assert::check(m_extractionBitstream->empty(), "Bitstream must be empty after block decoding");
    };
    m_blockDecoder->setAfter(check_empty_bitsteam);
}

void decoder::SinglePassDecoder::openIcovFile(const boost::filesystem::path &path)
{
    m_icovFile = std::make_shared<InputIcov>(path);
    m_icovFile->readContentParameterSet();
    setContentParams(m_icovFile->contentParameterSet());
    m_icovFile->readBlockPos();
}

void decoder::SinglePassDecoder::addDefaultNavigation()
{
    m_navigation = std::make_shared<factory::Navigation360>();

    const auto &cps = m_contentParams;

    const point2d resolution(cps.viewport_width, cps.viewport_height);
    const angle_radian fov(cps.viewport_fov_x, cps.viewport_fov_y);

    viewport_geometry vpg = factory::default_geometry(fov, resolution);

    std::vector<Block::id_t> visibles;
    std::transform(cps.encoded_blocks.cbegin(), cps.encoded_blocks.cend(), std::back_inserter(visibles),
                   [](const auto &pair) { return pair.first; });

    m_navigation->init(
        {

            .VisualizationParams =
                {

                    .Width = contentParams().content_width,

                    .Height = contentParams().content_height,

                    .DummyViewport = m_params.DummyViewport,

                    .ViewportPadding = cps.viewport_padding

                },

            .IcuSize = cps.block_fixed_size,

            .VisibleBlocks = visibles

        },
        vpg);

    m_navigation->Request->setBlockProcessor(m_blockProcessor);
}

void decoder::SinglePassDecoder::addDefaultExtractor()
{
    const auto &cps = m_contentParams;
    const ChromaFormat chFrmt = static_cast<ChromaFormat>(cps.content_chroma_format);

    const int channelCount = getNumberValidComponents(chFrmt);
    BlockExtractor::params extractor_params;

    const int icu_size = (int)cps.block_fixed_size;
    UnitArea blockArea(chFrmt, Area(0, 0, icu_size, icu_size));

    for (int c = 0; c < channelCount; c++)
    {
        const auto compID = static_cast<ComponentID>(c);
        extractor_params.icu_sizes.push_back(blockArea.block(compID).area());
    }

    setBlockExtractor(
        std::make_shared<BlockExtractor>(m_icovFile, extractor_params, ldpcParams(), m_extractionBitstream));
}

void decoder::SinglePassDecoder::addDefaultDecoder()
{

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

                }

        },
        ldpcParams(), m_extractionBitstream));
}

const PelUnitBuf decoder::SinglePassDecoder::getDecodedBuf() const
{
    icov::assert::not_null(m_blockDecoder.get(), "Decoder cannot be null");
    return m_blockDecoder->decodingOutput();
}

void decoder::SinglePassDecoder::start()
{
    m_navigation->Request->start();
    this->request(0, 0, 0);
}

void decoder::SinglePassDecoder::request(angle_t x, angle_t y, frame_t i)
{
    icov::assert::not_null(m_navigation.get(), "Navigation cannot be null");
    icov::assert::not_null(m_navigation->Request.get(), "Request manager cannot be null");

    VideoStreamDebugger::get_mutable_instance().reset();

    if (x != pov_x || y != pov_y || i != frame)
    {
        if (m_blockDecoder)
            m_blockDecoder->getDecodingOutput().fill(0);
        m_navigation->Request->update({.viewpoint = {.angleX = x, .angleY = y}, .frameID = i});
    }

  pov_x = x;
  pov_y = y;
  frame = i;
}

const LdpcCoderParameters &decoder::SinglePassDecoder::ldpcParams() const
{
  assert::not_null(m_ldpcParams.get(), "LDPC parameters cannot be null");
  return *m_ldpcParams;
}

void decoder::SinglePassDecoder::setLdpcParams(const LdpcCoderParameters &newLdpcParams)
{
  m_ldpcParams = newLdpcParams.clone();
}

const ContentParameterSet &decoder::SinglePassDecoder::contentParams() const
{
  return m_contentParams;
}

const std::shared_ptr<factory::Navigation360> &decoder::SinglePassDecoder::navigation() const
{
  return m_navigation;
}

void decoder::SinglePassDecoder::setNavigation(const std::shared_ptr<factory::Navigation360> &newNavigation)
{
  m_navigation = newNavigation;
}

const std::shared_ptr<InputIcov> &decoder::SinglePassDecoder::icovFile() const
{
  return m_icovFile;
}

void decoder::SinglePassDecoder::setIcovFile(const std::shared_ptr<InputIcov> &newIcovFile)
{
  m_icovFile = newIcovFile;
}

const sp_IcovExtractedBitstream &decoder::SinglePassDecoder::extractionBitstream() const
{
  return m_extractionBitstream;
}

void decoder::SinglePassDecoder::setContentParams(const ContentParameterSet &newContentParams)
{
  m_contentParams = newContentParams;
}

const decoder::SinglePassDecoder::params &decoder::SinglePassDecoder::getParams() const
{
  return m_params;
}

void decoder::SinglePassDecoder::setParams(const decoder::SinglePassDecoder::params &newParams)
{
  m_params = newParams;
}

const std::shared_ptr<BlockExtractor> &decoder::SinglePassDecoder::blockExtractor() const
{
  return m_blockExtractor;
}

const std::shared_ptr<decoder::BlockDecoder> &decoder::SinglePassDecoder::blockDecoder() const
{
  return m_blockDecoder;
}

const std::shared_ptr<IBlockProcessor> &decoder::SinglePassDecoder::blockProcessor() const
{
  return m_blockProcessor;
}

void decoder::SinglePassDecoder::setBlockExtractor(const std::shared_ptr<BlockExtractor> &newBlockExtractor)
{
  m_blockExtractor = newBlockExtractor;
  IBlockProcessor::addChild(m_blockProcessor, m_blockExtractor);
}

void decoder::SinglePassDecoder::setBlockDecoder(const std::shared_ptr<decoder::BlockDecoder> &newBlockDecoder)
{
  m_blockDecoder = newBlockDecoder;
  IBlockProcessor::addChild(m_blockProcessor, m_blockDecoder);
}
