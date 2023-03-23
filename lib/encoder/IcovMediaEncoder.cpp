#include "IcovMediaEncoder.h"
#include "encoder/BinaryOutputIcov.h"
#include "encoder/FrameEncoder.h"
#include "encoder/TextOutputIcov.h"
#include "helpers.h"
#include "omnidirectional_layer/factory.h"
#include <exception>
#include <memory>

/*****************************************************************************/
icov::encoder::IcovMediaEncoder::IcovMediaEncoder(
    const params &p, const LdpcCoderParameters &LDPC_params)
    : m_params(p), m_LDPC_params(LDPC_params.clone()) {
  m_LDPC_params->n = m_params.FrameEncoderParams.ICU_Size.area();
  createOutputFile();
}

/*****************************************************************************/
icov::encoder::IcovMediaEncoder::~IcovMediaEncoder() {
  std::cout << "destroy IcovMediaEncoder" << std::endl;
}

void encoder::IcovMediaEncoder::defaultEncode(float viewportPadding) {

  std::unique_ptr<icov::encoder::InputMedia> input;

  icov::encoder::instantiateInputMedia(m_params.InputPath, input);

  BlockPrediction icov_blockPrediction;
  icov::factory::Visualization360 icov_visualization(
      icov::factory::Visualization360::params{

          .Width = static_cast<uint>(input->getWidth()),

          .Height = static_cast<uint>(input->getHeight()),

          .ViewportPadding = viewportPadding

      });

  m_output->contentParameterSet().content_width =
      static_cast<uint16_t>(input->getWidth());
  m_output->contentParameterSet().content_height =
      static_cast<uint16_t>(input->getHeight());

  m_output->contentParameterSet().icov_version = "icov_test" ICOV_CORE_VERSION;
  m_output->contentParameterSet().content_framecount = 1;
  m_output->contentParameterSet().is_block_fixed_size = true;
  m_output->contentParameterSet().block_fixed_size =
      static_cast<icov::byte>(32);
  m_output->contentParameterSet().content_chroma_format =
      (icov::byte)m_params.FrameEncoderParams.ChFormat;

  m_output->writeContentParameterSet();
  this->encode(*input, *icov_visualization.Picture, icov_blockPrediction);
}

void icov::encoder::IcovMediaEncoder::processInput(
    const icov::IInputMedia<cv::Mat> &input) {}

std::unique_ptr<icov::encoder::IFrameEncoder<cv::Mat>>
icov::encoder::IcovMediaEncoder::createFrameEncoder() const {

  auto ptr = std::make_unique<FrameEncoder>(m_params.FrameEncoderParams,
                                            *m_LDPC_params);
  ptr->setIcovFile(m_output);
  return ptr;
}

std::unique_ptr<encoder::OutputIcovBase>
encoder::IcovMediaEncoder::createOutputFile() const {
  std::unique_ptr<encoder::OutputIcovBase> ptr;
  switch (m_params.OutputType) {
  case BINARY:
    ptr = std::make_unique<encoder::BinaryOutputIcov>(m_params.OutputPath);
    break;
  case TEXT:
    ptr = std::make_unique<encoder::TextOutputIcov>(m_params.OutputPath);
    break;
  default:
    ICOV_THROW_INVALID_ARGUMENT("Unknown encoder type");
  }

  icov::helpers::assert::not_null(ptr.get());

  return ptr;
}
