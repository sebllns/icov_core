#include "InteractiveDecoder.h"

#include "app_common/program/icov_program_options.h"
#include "icov_assert.h"
#include "icov_log.h"
#include "icov_math.h"
#include "icov_path.h"
#include "icov_request.h"
#include "video_layer/VideoMetrics.h"
#include "video_layer/vtm_opencv_utility.h"

#include "opencv2/core/types.hpp"

#include <cstdio>
#include <fstream>
#include <memory>
#include <string>

InteractiveDecoder::InteractiveDecoder(const path::path_t &icov_file, const options::args_t &args)
    : InteractiveModeBase(), m_decoderCtrl(icov_file, args)
{
    this->m_config.f1 = -66;
    this->m_config.f2 = -65;
    this->m_config.f3 = -64;
    this->m_config.f4 = -63;
    this->m_config.f5 = -62;

    this->init360Viewer(getDecoder()->contentParams().viewport_width,
                        getDecoder()->contentParams().viewport_height,
                        getDecoder()->contentParams().viewport_fov_y);

    m_decoderCtrl.initAnalyzer();
    m_requests = m_decoderCtrl.readRequests();
}

const icov_unique_ptr(decoder::SinglePassDecoder) &
    InteractiveDecoder::getDecoder() const {
    return m_decoderCtrl.getDecoder();
}

void InteractiveDecoder::updateF1()
{
    VTM_OPENCV_Utility::toBGRMat(getDecoder()->getDecodedBuf(), m_erpImage);
}

void InteractiveDecoder::updateF2() {
    std::cout << "update f2 \n";

    this->fill(bgrColor::BLACK());
    const auto pic = getDecoder()->navigation()->Visualization->Picture;

    for (const auto &id :
         icov::video_layer::VideoMetrics::get_const_instance().visible_blocks) {
      const auto b = pic->getById(id);
      const auto &i =
          icov::video_layer::VideoMetrics::get_const_instance().getBlock(*b);
      assert::check(pic->isValidBlock(b), "Block not valid");
      std::cout << b->center_u() << ", " << b->center_v() << " = " << i.rate()
                << std::endl;

      int bw = (int)(b->w() * pic->width());
      int bh = (int)(b->h() * pic->height());
      int bx = (int)(pic->u(pic->angleX(b->u())) * pic->width());
      int by = (int)(pic->v(pic->angleY(b->v())) * pic->height());
      auto rect_block = cv::Rect(bx, by, bw, bh);
      cv::Scalar color = i.rate() * ((i.prediction > 0 && i.rate() >= 0.5)
                                         ? bgrColor().RED()
                                         : bgrColor().WHITE());
      this->m_erpImage(rect_block).setTo(color);
    }
}

void InteractiveDecoder::updateF3()
{
    std::cout << "update f3 \n";

    this->fill(bgrColor::BLACK());
    const auto pic = getDecoder()->navigation()->Visualization->Picture;

    Rate bitrate = icov::video_layer::VideoMetrics::get_const_instance().viewportRate();
    cv::Scalar color = bitrate * bgrColor().WHITE();

    for (const auto &id :
         icov::video_layer::VideoMetrics::get_const_instance().visible_blocks) {
      const auto b = pic->getById(id);
      const auto &i =
          icov::video_layer::VideoMetrics::get_const_instance().getBlock(*b);
      assert::check(pic->isValidBlock(b), "Block not valid");

      int bw = (int)(b->w() * pic->width());
      int bh = (int)(b->h() * pic->height());
      int bx = (int)(pic->u(pic->angleX(b->u())) * pic->width());
      int by = (int)(pic->v(pic->angleY(b->v())) * pic->height());
      auto rect_block = cv::Rect(bx, by, bw, bh);
      this->m_erpImage(rect_block).setTo(color);
    }
}

void InteractiveDecoder::updateF4()
{
    std::cout << "update f4 \n";

    this->fill(bgrColor::BLACK());
    const auto pic = getDecoder()->navigation()->Visualization->Picture;

    for (const auto &id :
         icov::video_layer::VideoMetrics::get_const_instance().visible_blocks) {
      const auto b = pic->getById(id);
      const auto &i =
          icov::video_layer::VideoMetrics::get_const_instance().getBlock(*b);
      assert::check(pic->isValidBlock(b), "Block not valid");

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

      switch (BlockPrediction::toPredType(i.prediction)) {
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

      this->m_erpImage(rect_block).setTo(color);
    }
}

void InteractiveDecoder::updateF5()
{
    std::cout << "update f5 \n";

    this->fill(bgrColor::BLACK());
    const auto pic = getDecoder()->navigation()->Visualization->Picture;

    for (const auto &id :
         icov::video_layer::VideoMetrics::get_const_instance().visible_blocks) {
      const auto b = pic->getById(id);
      const auto &i =
          icov::video_layer::VideoMetrics::get_const_instance().getBlock(*b);
      assert::check(pic->isValidBlock(b), "Block not valid");
      // std::cout << b->center_u() << ", " << b->center_v() << " = " << i.rate
      // << std::endl;

      int bw = (int)(b->w() * pic->width());
      int bh = (int)(b->h() * pic->height());
      int bx = (int)(pic->u(pic->angleX(b->u())) * pic->width());
      int by = (int)(pic->v(pic->angleY(b->v())) * pic->height());
      auto rect_block = cv::Rect(bx, by, bw, bh);
      cv::Scalar color = bgrColor::BLACK();

      switch (BlockPrediction::getCategory(
          BlockPrediction::toPredType(i.prediction))) {
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

      this->m_erpImage(rect_block).setTo(color);
    }
}

bool InteractiveDecoder::processKey(char key)
{
    if (!InteractiveModeBase::processKey(key))
        return false;

    angle_t pov_x = getPov_x_DEG();
    angle_t pov_y = getPov_y_DEG();
    frame_t frame = 0;

    setWait_delay(0);

    if (!m_requests.empty())
    {
        pov_x = m_requests.front().viewpoint.angleX;
        pov_y = m_requests.front().viewpoint.angleY;
        frame = m_requests.front().frameID;

        m_requests.pop();
        setWait_delay(1);
    }

    ICOV_DEBUG << "POV = " << pov_x << ", " << pov_y;

    // decode
    getDecoder()->request(DEG2RAD * pov_x, DEG2RAD * pov_y, frame);

    return true;
}
