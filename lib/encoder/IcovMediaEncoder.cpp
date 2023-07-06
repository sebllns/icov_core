#include "IcovMediaEncoder.h"
#include "encoder/BinaryOutputIcov.h"
#include "encoder/FrameEncoder.h"
#include "encoder/InputMedia.h"
#include "encoder/TextOutputIcov.h"

#include "icov_log.h"
#include "omnidirectional_layer/factory.h"
#include "omnidirectional_layer/geometry.h"
#include <exception>
#include <memory>

using namespace icov;
using namespace icov::encoder;
using namespace icov::omnidirectional_layer;

// ICU_MAX_COUNT: max block ID to encode (useful for debug), 0 is special value
// meaning ALL BLOCKS
#ifndef ICU_MAX_COUNT
#define ICU_MAX_COUNT 0
#endif

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
    float viewportPadding, const viewport_geometry &vpg) {

  std::unique_ptr<icov::encoder::InputMedia> input;

  icov::encoder::instantiateInputMedia(m_params.InputPath, input);

  omnidirectional_layer::BlockPrediction icov_blockPrediction;
  omnidirectional_layer::factory::Visualization360 icov_visualization(
      omnidirectional_layer::factory::Visualization360::params{

          .Width = static_cast<uint>(input->getWidth()),

          .Height = static_cast<uint>(input->getHeight()),

          .ViewportPadding = viewportPadding

      });

  icov::assert::equal(m_params.FrameEncoderParams.ICU_Size.width,
                      m_params.FrameEncoderParams.ICU_Size.height,
                      "Assert square ICU");

  icov_visualization.init(m_params.FrameEncoderParams.ICU_Size.width, vpg);

  m_output->contentParameterSet().content_width =
      static_cast<uint16_t>(input->getWidth());
  m_output->contentParameterSet().content_height =
      static_cast<uint16_t>(input->getHeight());

  m_output->contentParameterSet().icov_version = "icov_test" ICOV_CORE_VERSION;
  m_output->contentParameterSet().content_framecount = 1;
  m_output->contentParameterSet().is_block_fixed_size = true;
  m_output->contentParameterSet().block_fixed_size =
      static_cast<bitstream::byte>(32);
  m_output->contentParameterSet().content_chroma_format =
      (bitstream::byte)m_params.FrameEncoderParams.ChFormat;
  m_output->contentParameterSet().icu_count = ICU_MAX_COUNT;

  if (ICU_MAX_COUNT == 0) {
    ICOV_INFO << "All blocks will be encoded";
  } else {
    ICOV_INFO << "Max ICU ID to encode = " << ICU_MAX_COUNT;
  }

  m_output->writeContentParameterSet();
  this->encode(*input, *icov_visualization.Picture, icov_blockPrediction);
}

/*****************************************************************************/
void icov::encoder::IcovMediaEncoder::processInput(
    const icov::IInputMedia<cv::Mat> &input) {}

/*****************************************************************************/
std::unique_ptr<icov::encoder::IFrameEncoder<cv::Mat>>
icov::encoder::IcovMediaEncoder::createFrameEncoder() const {

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
