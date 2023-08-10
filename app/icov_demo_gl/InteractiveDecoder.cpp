#include "InteractiveDecoder.h"
#include "InteractiveModeBase.h"
#include "app_common/program/icov_program_options.h"
#include "icov_assert.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "opencv2/core/types.hpp"
#include "video_layer/vtm_opencv_utility.h"

InteractiveDecoder::InteractiveDecoder(const params &params, const LdpcCoderParameters &ldpc_params)
    : InteractiveModeBase(), m_params(params)
{
    path::assert_file_exists(m_params.icov_file.string());
    m_decoder = std::make_unique<icov::decoder::SinglePassDecoder>();
    m_decoder->initLocal(decoder::SinglePassDecoder::params{.DummyViewport = m_params.dummyViewport},
                         m_params.icov_file, ldpc_params);
    m_results = m_decoder->addDefaultResults();
    m_decoder->start();
    this->m_config.f1 = -66;
    this->m_config.f2 = -65;
    this->m_config.f3 = -64;
    this->m_config.f4 = -63;
}

InteractiveDecoder::params::params(const boost::program_options::variables_map &args)
{
    dummyViewport = false;
    viewportPadding = 1.2f;
    icov_file = options::parseInputFile(args);
}


void InteractiveDecoder::updateF1()
{
    VTM_OPENCV_Utility::toBGRMat(m_decoder->getDecodedBuf(), m_outputImage);
}

void InteractiveDecoder::updateF2()
{
    std::cout << "update f2 \n";

    this->fill(bgrColor::BLACK());
    const auto pic = m_decoder->navigation()->Visualization->Picture;

    for (const auto &i : m_results->infos())
    {
        const auto b = pic->getById(i.id);
        assert::check(pic->isValidBlock(b), "Block not valid");
        std::cout << b->center_u() << ", " << b->center_v() << " = " << i.rate << std::endl;

        int bw = (int)(b->w() * pic->width());
        int bh = (int)(b->h() * pic->height());
        int bx = (int)(pic->u(pic->angleX(b->u())) * pic->width());
        int by = (int)(pic->v(pic->angleY(b->v())) * pic->height());
        auto rect_block = cv::Rect(bx, by, bw, bh);
        cv::Scalar color = i.rate * bgrColor().GREEN();
        this->m_outputImage(rect_block).setTo(color);
    }
}

void InteractiveDecoder::updateF3()
{
    std::cout << "update f3 \n";
    this->fill(bgrColor::BLACK());
    const auto pic = m_decoder->navigation()->Visualization->Picture;

    for (const auto &i : m_results->infos())
    {
        const auto b = pic->getById(i.id);
        assert::check(pic->isValidBlock(b), "Block not valid");
        // std::cout << b->center_u() << ", " << b->center_v() << " = " << i.rate << std::endl;

        int bw = (int)(b->w() * pic->width());
        int bh = (int)(b->h() * pic->height());
        int bx = (int)(pic->u(pic->angleX(b->u())) * pic->width());
        int by = (int)(pic->v(pic->angleY(b->v())) * pic->height());
        auto rect_block = cv::Rect(bx, by, bw, bh);
        cv::Scalar color = bgrColor::BLACK();

        switch (BlockPrediction::getCategory(BlockPrediction::toPredType(i.prediction)))
        {
        case icov::omnidirectional_layer::BlockPrediction::SI_CAT_COUNT:
        case icov::omnidirectional_layer::BlockPrediction::SI_CAT_UNKNOWN:
            color = bgrColor::RED();
            break;
        case icov::omnidirectional_layer::BlockPrediction::SI_CAT_NONE:
            color = bgrColor::GREEN();
            break;
        case icov::omnidirectional_layer::BlockPrediction::SI_CAT_SINGLE:
            color = bgrColor::BLUE();
            break;
        case icov::omnidirectional_layer::BlockPrediction::SI_CAT_DOUBLE:
            color = bgrColor::CYAN();
            break;
        case icov::omnidirectional_layer::BlockPrediction::SI_CAT_TEMPORAL:
            color = bgrColor::YELLOW();
            break;
        }

        this->m_outputImage(rect_block).setTo(color);
    }
}

void InteractiveDecoder::updateF4()
{
    std::cout << "update f4 \n";

    this->fill(bgrColor::BLACK());
    const auto pic = m_decoder->navigation()->Visualization->Picture;

    for (const auto &i : m_results->infos())
    {
        const auto b = pic->getById(i.id);
        assert::check(pic->isValidBlock(b), "Block not valid");
        std::cout << b->center_u() << ", " << b->center_v() << " = " << i.rate << std::endl;

        int bw = (int)(b->w() * pic->width());
        int bh = (int)(b->h() * pic->height());
        int bx = (int)(pic->u(pic->angleX(b->u())) * pic->width());
        int by = (int)(pic->v(pic->angleY(b->v())) * pic->height());
        auto rect_block = cv::Rect(bx, by, bw, bh);
        cv::Scalar color = bgrColor::BLACK();

        cv::Scalar r_color(127, 127, 0);
        cv::Scalar l_color(0, 127, 127);
        cv::Scalar t_color(127, 0, 127);
        cv::Scalar b_color(127, 127, 127);

        switch (BlockPrediction::toPredType(i.prediction))
        {
        case icov::omnidirectional_layer::BlockPrediction::RIGHT:
            color = r_color;
            break;
        case icov::omnidirectional_layer::BlockPrediction::LEFT:
            color = l_color;
            break;
        case icov::omnidirectional_layer::BlockPrediction::TOP:
            color = t_color;
            break;
        case icov::omnidirectional_layer::BlockPrediction::BOTTOM:
            color = b_color;
            break;
        case icov::omnidirectional_layer::BlockPrediction::TOP_RIGHT:
            color = t_color + r_color;
            break;
        case icov::omnidirectional_layer::BlockPrediction::TOP_LEFT:
            color = t_color + l_color;
            break;

        case icov::omnidirectional_layer::BlockPrediction::BOTTOM_RIGHT:
            color = b_color + r_color;
            break;

        case icov::omnidirectional_layer::BlockPrediction::BOTTOM_LEFT:
            color = b_color + l_color;
            break;

        case icov::omnidirectional_layer::BlockPrediction::NONE:
            color = bgrColor::GREEN();
            break;

        case icov::omnidirectional_layer::BlockPrediction::INTER:
            color = bgrColor::YELLOW();
            break;

        case icov::omnidirectional_layer::BlockPrediction::UNKNOWN:
        case icov::omnidirectional_layer::BlockPrediction::MAX:
        case icov::omnidirectional_layer::BlockPrediction::ERROR_PRED:
            color = bgrColor::RED();
            break;
        }

        this->m_outputImage(rect_block).setTo(color);
    }
}

bool InteractiveDecoder::processKey(char key)
{
    if (!InteractiveModeBase::processKey(key))
        return false;

    float pov_x = getPov_x();
    float pov_y = getPov_y();
    int frame = 0;

    // decode
    m_decoder->request(pov_x, pov_y, frame);

    draw();

    return true;
}
