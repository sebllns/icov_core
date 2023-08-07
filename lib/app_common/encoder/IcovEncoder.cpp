#include "IcovEncoder.h"
#include "app_common/program/icov_program_options.h"
#include "icov_path.h"
#include "omnidirectional_layer/factory.h"
#include <boost/filesystem/path.hpp>

icov::encoder::IcovEncoder::IcovEncoder(const params &params, const LdpcCoderParameters &ldpc_params)
    : m_params(params), m_ldpc_params(ldpc_params.clone())
{
    path::assert_file_exists(m_params.m_inputfile);

    m_ldpc_params->codeLength = m_params.m_icuSize * m_params.m_icuSize;

    bitstream::VideoEncodingStreamCache::get_mutable_instance().clear();
    bitstream::VideoStreamDebugger::get_mutable_instance().reset();
    bitstream::VideoEncodingStreamCache::get_mutable_instance().setFrameCount(m_params.m_debugframecount);

    m_encoder = std::make_unique<encoder::IcovMediaEncoder>(
        encoder::IcovMediaEncoder::params{
            .InputPath = m_params.m_inputfile,
            .OutputPath = m_params.m_outputfile,
            .OutputType = encoder::IcovMediaEncoder::BINARY,
            .FrameEncoderParams =
                icov::encoder::FrameEncoder::params{.QP = static_cast<int>(m_params.m_qp),
                                                    .ChFormat = CHROMA_420,
                                                    .ICU_Size = cv::Size(m_params.m_icuSize, m_params.m_icuSize),
                                                    .BSP_Params = m_params.bsp_params,
                                                    .CheckDecoding = false,
                                                    .MaxRateNoCoding = 4},
            .AllBlocks = true,
            .StartBlock = 61,
            .BlockCount = 23,
            .DummyViewport = m_params.m_DummyViewport},
        this->ldpc_params());
}

icov::encoder::IcovEncoder::~IcovEncoder()
{
}

void icov::encoder::IcovEncoder::run()
{
    viewport_geometry vpg = factory::default_geometry(m_params.fov, m_params.resolution);
    m_encoder->defaultEncode(m_params.viewport_padding, vpg);
}

const icov::LdpcCoderParameters &icov::encoder::IcovEncoder::ldpc_params() const
{
    return *m_ldpc_params;
}

const icov::encoder::IcovEncoder::params &icov::encoder::IcovEncoder::getParams() const
{
    return m_params;
}

const boost::filesystem::path &icov::encoder::IcovEncoder::inputfile() const
{
    return m_params.m_inputfile;
}

const boost::filesystem::path &icov::encoder::IcovEncoder::outputfile() const
{
    return m_params.m_outputfile;
}

icov::encoder::IcovEncoder::params::params(const boost::program_options::variables_map &args)
{
    m_debugframecount = 100;
    m_DummyViewport = false;
    m_qp = 42;
    m_icuSize = 32;

    viewport_padding = 1.2;
    fov_x_degree = 60;
    resolution = point2d(1920, 1080);
    fov = factory::fov(fov_x_degree, factory::y_x_ratio(resolution));

    bsp_params.enable_z_quantization = true;
    icov::io::read_vector(ICOV_Q_VALUES_FILE(args).string(), bsp_params.q_values);

    m_inputfile = options::parseInputFile(args);
    m_outputfile = options::parseOutputFile(args);
    if (m_outputfile.empty())
        m_outputfile = fs::path(m_inputfile).replace_extension(".icov").string();
}
