#include "InteractiveDecoder.h"
#include "app_common/program/icov_program_options.h"
#include "video_layer/vtm_opencv_utility.h"

InteractiveDecoder::InteractiveDecoder(const params &params, const LdpcCoderParameters &ldpc_params)
    : InteractiveModeBase(), m_params(params)
{
    path::assert_file_exists(m_params.icov_file.string());
    m_decoder = std::make_unique<icov::decoder::SinglePassDecoder>(m_params.icov_file);
    m_decoder->initialize(decoder::SinglePassDecoder::params{.DummyViewport = m_params.dummyViewport},

                          ldpc_params);
}

bool InteractiveDecoder::processKey(char key)
{
    if (!InteractiveModeBase::processKey(key))
        return false;

    // decode
    float pov_x = getPov_x();
    float pov_y = getPov_y();
    int frame = 0;
    m_decoder->request(pov_x, pov_y, frame);
    VTM_OPENCV_Utility::toBGRMat(m_decoder->getOutputBuf(), m_outputImage);

    // process custom keys

    return true;
}

InteractiveDecoder::params::params(const boost::program_options::variables_map &args)
{
    dummyViewport = false;
    viewportPadding = 1.2f;
    icov_file = options::parseInputFile(args);
}
