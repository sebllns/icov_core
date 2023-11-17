#include "IcovMediaEncoder.h"
#include "encoder/BinaryOutputIcov.h"
#include "encoder/FakeFrameEncoder.h"
#include "encoder/FrameEncoder.h"
#include "encoder/InputMedia.h"
#include "encoder/TextOutputIcov.h"

#include "icov_log.h"
#include "omnidirectional_layer/factory.h"
#include "omnidirectional_layer/icov_geometry.h"
#include <exception>
#include <memory>

using namespace icov;
using namespace icov::encoder;
using namespace icov::omnidirectional_layer;

// ICU_MAX_COUNT: max block ID to encode (useful for debug), 0 is special value
// meaning ALL BLOCKS
//#ifndef ICU_MAX_COUNT
//#define ICU_MAX_COUNT 0
//#endif

/*****************************************************************************/
icov::encoder::IcovMediaEncoder::IcovMediaEncoder(
    const params &p, const LdpcCoderParameters &LDPC_params)
    : m_params(p), m_LDPC_params(LDPC_params.clone()) {
  m_LDPC_params->codeLength = m_params.FrameEncoderParams.ICU_Size.area();
  m_output = createOutputFile();
}

/*****************************************************************************/
icov::encoder::IcovMediaEncoder::~IcovMediaEncoder() {
  std::cout << "destroy IcovMediaEncoder" << std::endl;
}

/*****************************************************************************/
void icov::encoder::IcovMediaEncoder::defaultEncode(
    float fov_x_degree, float viewportPadding, float display_horizontal_ratio,
    float viewport_image_format) {

  std::unique_ptr<icov::encoder::InputMedia> input;

  icov::encoder::instantiateInputMedia(m_params.InputPath, input);

  omnidirectional_layer::BlockPrediction icov_blockPrediction;
  omnidirectional_layer::factory::Visualization360 icov_visualization(
      omnidirectional_layer::factory::Visualization360::params{

          .Width = static_cast<uint>(input->getWidth()),

          .Height = static_cast<uint>(input->getHeight()),

          .DummyViewport = m_params.DummyViewport,

          .ViewportPadding = viewportPadding

      });

  icov::assert::equal(m_params.FrameEncoderParams.ICU_Size.width,
                      m_params.FrameEncoderParams.ICU_Size.height,
                      "Assert square ICU");

  float y_x_format = 1.0f / viewport_image_format;
  angle_radian fov = factory::fov(fov_x_degree, y_x_format);
  int vp_width =
      static_cast<int>(floor(input->getWidth() * display_horizontal_ratio));
  int vp_height = static_cast<int>(floor(vp_width * y_x_format));

  point2d viewportResolution(vp_width, vp_height);

  viewport_geometry vpg = factory::default_geometry(fov, viewportResolution);

  icov_visualization.init(m_params.FrameEncoderParams.ICU_Size.width, vpg);

  // select blocks to encode
  size_t block_counter = 0;
  for (auto &b : icov_visualization.Picture->icus()) {
    if (m_params.AllBlocks ||
        (b.getIdSelf() >= m_params.StartBlock &&
         b.getIdSelf() < (m_params.StartBlock + m_params.BlockCount))) {
      icov_visualization.Picture->getById(b.getIdSelf())->setHidden(false);
      m_output->contentParameterSet().encoded_blocks[b.getIdSelf()] = block_counter;
      ++block_counter;
    } else {
      icov_visualization.Picture->getById(b.getIdSelf())->setHidden(true);
    }
  }

  icov_visualization.AccessBlockManager->buildAccessBlocks(
      *icov_visualization.Picture, *icov_visualization.ViewportManager);

  m_output->contentParameterSet().content_width =
      static_cast<uint16_t>(input->getWidth());
  m_output->contentParameterSet().content_height =
      static_cast<uint16_t>(input->getHeight());

  m_output->contentParameterSet().viewport_width =
      static_cast<uint16_t>(vpg.resolution.x());
  m_output->contentParameterSet().viewport_height = static_cast<uint16_t>(vpg.resolution.y());

  m_output->contentParameterSet().qp = m_params.FrameEncoderParams.QP;

  m_output->contentParameterSet().viewport_fov_x = vpg.fov.x();
  m_output->contentParameterSet().viewport_fov_y = vpg.fov.y();

  m_output->contentParameterSet().viewport_padding = viewportPadding;

  m_output->contentParameterSet().icov_version = "icov_" ICOV_CORE_VERSION;

  m_output->contentParameterSet().content_framecount = 0;

  m_output->contentParameterSet().is_block_fixed_size = (m_params.FrameEncoderParams.ICU_Size.aspectRatio() == 1);

  m_output->contentParameterSet().block_fixed_size = static_cast<byte>(m_params.FrameEncoderParams.ICU_Size.width);

  m_output->contentParameterSet().content_chroma_format = static_cast<byte>(m_params.FrameEncoderParams.ChFormat);

  if (m_params.AllBlocks) {
    ICOV_INFO << "All blocks will be encoded";
    m_output->contentParameterSet().icu_count =
        icov_visualization.Picture->blockCount();
  } else {
    ICOV_INFO << "ICU count to encode = " << m_params.BlockCount;
    m_output->contentParameterSet().icu_count = m_params.BlockCount;
  }

  ICOV_INFO << "Start encoding media";
  this->encode(*input, *icov_visualization.Picture, icov_blockPrediction);
  m_output->writeContentParameterSet();
  m_output->flush(bitstream::eIcovStream::COUNT);
  ICOV_INFO << "End encoding media";
}

/*****************************************************************************/
void icov::encoder::IcovMediaEncoder::processInput(
    const icov::IInputMedia<cv::Mat> &input) {}

/*****************************************************************************/
std::unique_ptr<icov::encoder::IFrameEncoder<cv::Mat>>
icov::encoder::IcovMediaEncoder::createFrameEncoder() const {

  // auto ptr = std::make_unique<FakeFrameEncoder>();

  auto ptr = std::make_unique<FrameEncoder>(m_params.FrameEncoderParams,
                                            *m_LDPC_params);
  ptr->setIcovFile(m_output);
  return ptr;
}

/*****************************************************************************/
std::unique_ptr<icov::encoder::OutputIcovBase>
icov::encoder::IcovMediaEncoder::createOutputFile() const {
  std::unique_ptr<encoder::OutputIcovBase> ptr;
  switch (m_params.OutputType) {
  case BINARY:
    ICOV_INFO << "Icov output format is binary";
    ptr = std::make_unique<encoder::BinaryOutputIcov>(m_params.OutputPath);
    break;
  case TEXT:
    ICOV_INFO << "Icov output format is text";
    ptr = std::make_unique<encoder::TextOutputIcov>(m_params.OutputPath);
    break;
  default:
    ICOV_THROW_INVALID_ARGUMENT("Unknown encoder type");
  }

  icov::assert::not_null(ptr.get());

  return ptr;
}

/*****************************************************************************/
const icov::encoder::IcovMediaEncoder::params &
icov::encoder::IcovMediaEncoder::parameters() const {
  return m_params;
}

/*****************************************************************************/
const LdpcCoderParameters &
icov::encoder::IcovMediaEncoder::ldpc_parameters() const {
  return *m_LDPC_params;
}

/*****************************************************************************/
const encoder::OutputIcovBase &
icov::encoder::IcovMediaEncoder::output_media() const {
  return *m_output;
}
