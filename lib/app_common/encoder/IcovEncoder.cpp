#include "IcovEncoder.h"
#include "app_common/program/icov_program_options.h"
#include "boost/format.hpp"
#include "icov_path.h"
#include "omnidirectional_layer/factory.h"
#include <boost/filesystem/path.hpp>

icov::encoder::IcovEncoder::IcovEncoder(const params &params,
                                        const LdpcCoderParameters &ldpc_params)
    : m_params(params), m_ldpc_params(ldpc_params.clone()) {
  // path::assert_file_exists(m_params.Inputfile);

  m_ldpc_params->codeLength = m_params.m_icuSize * m_params.m_icuSize;

  bitstream::VideoEncodingStreamCache::get_mutable_instance().clear();
  bitstream::VideoStreamDebugger::get_mutable_instance().reset();
  bitstream::VideoEncodingStreamCache::get_mutable_instance().setFrameCount(
      m_params.m_debugframecount);

  m_encoder = std::make_unique<encoder::IcovMediaEncoder>(
      encoder::IcovMediaEncoder::params{
          .InputPath = m_params.Inputfile,
          .OutputPath = m_params.Outputfile,
          .OutputType = encoder::IcovMediaEncoder::BINARY,
          .FrameEncoderParams =
              icov::encoder::FrameEncoder::params{
                  .QP = static_cast<int>(m_params.QP),
                  .ChFormat = CHROMA_420,
                  .ICU_Size = cv::Size(m_params.m_icuSize, m_params.m_icuSize),
                  .BSP_Params = m_params.bsp_params,
                  .MaxRateNoCoding = 8,
                  .ExportRecoFrames = m_params.ExportReference},
          .AllBlocks = true, //@todo TODO !!!
          .StartBlock = 61,
          .BlockCount = 23,
          .DummyViewport = m_params.DummyViewport},
      this->ldpc_params());

  // m_encoder->output_ptr()->stream() <<
}

icov::encoder::IcovEncoder::~IcovEncoder() {}

void icov::encoder::IcovEncoder::run() {
  ICOV_INFO << "-------------------";
  ICOV_INFO << "| STARTING ENCODER |";
  ICOV_INFO << "-------------------";

  ICOV_INFO << boost::format("%-15s %-s") % "Input File:" % m_params.Inputfile;
  ICOV_INFO << boost::format("%-15s %-s ") % "Output File:" %
                   m_params.Outputfile;

  m_encoder->setStartFrame(getParams().FrameStart);
  m_encoder->setMaxFrameCount(getParams().FrameCount);
  m_encoder->defaultEncode(m_params.fov_x_degree, m_params.ViewportPadding,
                           m_params.display_horizontal_ratio,
                           m_params.viewport_image_format);

  ICOV_INFO << "--------------------";
  ICOV_INFO << "| STOPPING ENCODER |";
  ICOV_INFO << "--------------------";
}

const icov::LdpcCoderParameters &
icov::encoder::IcovEncoder::ldpc_params() const {
  return *m_ldpc_params;
}

const icov::encoder::IcovEncoder::params &
icov::encoder::IcovEncoder::getParams() const {
  return m_params;
}

boost::filesystem::path icov::encoder::IcovEncoder::inputfile() const {
  return m_params.Inputfile;
}

boost::filesystem::path icov::encoder::IcovEncoder::outputfile() const {
  return m_params.Outputfile;
}

icov::encoder::IcovEncoder::params::params(
    const boost::program_options::variables_map &args) {

  m_debugframecount = 100; //@todo TODO put this at parameter
  DummyViewport = options::parseDummyViewportOption(args);
  QP = options::parseQP(args);
  m_icuSize = options::parseEncodeIcuSizeOption(args);

  ViewportPadding = options::parseViewportPadding(args);
  fov_x_degree = options::parseFov(args);

  display_horizontal_ratio = options::parseViewportErpRatio(args);
  viewport_image_format = options::parseViewportImageRatio(args);

  FrameCount = options::parseFrameCount(args);
  FrameStart = options::parseFrameStart(args);

  bsp_params.enable_z_quantization = true;
  icov::io::read_vector(ICOV_Q_VALUES_FILE(args).string(), bsp_params.q_values);

  Inputfile = options::parseInputFile(args);
  Outputfile = options::parseOutputFile(args);
  ExportReference = options::parseExportReference(args);
  // -o names the ICOV file; default: the input name with .icov
  if (Outputfile.empty())
    Outputfile = fs::path(Inputfile).replace_extension(".icov").string();
  else if (Outputfile.back() == '/' || fs::is_directory(Outputfile))
    ICOV_THROW_INVALID_ARGUMENT("-o expects a file name, e.g. out.icov, got " +
                                Outputfile);
}
