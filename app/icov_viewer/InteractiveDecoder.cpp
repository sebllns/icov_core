#include "InteractiveDecoder.h"

#include "app_common/program/icov_program_options.h"
#include "icov_assert.h"
#include "icov_log.h"
#include "icov_math.h"
#include "icov_path.h"
#include "icov_request.h"
#include "opencv2/core/persistence.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "video_layer/VideoMetrics.h"
#include "video_layer/vtm_opencv_utility.h"

#include "opencv2/core/types.hpp"

#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <unistd.h>

InteractiveDecoder::InteractiveDecoder(const path::path_t &icov_file,
                                       const options::args_t &args)
    : InteractiveModeBase(), m_decoderCtrl(icov_file, args),
      m_showSource(false) {
  this->m_config.f1 = -66;
  this->m_config.f2 = -65;
  this->m_config.f3 = -64;
  this->m_config.f4 = -63;
  this->m_config.f5 = -62;
  this->m_config.f6 = -61;
  this->m_config.custom_keys.insert(std::pair("show_source", (int)'x'));

  this->init360Viewer(getDecoder()->contentParams().viewport_width,
                      getDecoder()->contentParams().viewport_height,
                      getDecoder()->contentParams().viewport_fov_y * RAD2DEG);

  m_decoderCtrl.initAnalyzer();
  m_requests = m_decoderCtrl.readRequests();

  m_vp_rate_sum = 0;
  m_vp_psnr_sum = 0;
  m_erp_rate_sum = 0;
  m_erp_psnr_sum = 0;
  m_req_count = 0;
  frame = 0;
}

const icov_unique_ptr(decoder::SinglePassDecoder) &
    InteractiveDecoder::getDecoder() const {
    return m_decoderCtrl.getDecoder();
}

const decoder::DecoderController &InteractiveDecoder::decoderCtrl() const {
    return m_decoderCtrl;
}

void InteractiveDecoder::drawBlocks(drawType type) {
    const auto pic = getDecoder()->navigation()->Visualization->Picture;

    for (const auto &id :
         icov::video_layer::VideoMetrics::get_const_instance().visible_blocks) {
      const auto b = pic->getById(id);
      const auto &i =
          icov::video_layer::VideoMetrics::get_const_instance().getBlock(*b);
      assert::check(pic->isValidBlock(b), "Block not valid");
      //      std::cout << b->center_u() << ", " << b->center_v() << " = " <<
      //      i.rate()
      //                << std::endl;

      auto rect_block = VTM_OPENCV_Utility::getBlockRoi(*b);
      auto roi = this->m_erpImage(rect_block);

      drawBlock(type, *b, i, roi);
    }
}

void InteractiveDecoder::drawBlock(
    drawType type, const Block &b,
    const video_layer::VideoMetrics::block_infos_t &i, cv::Mat &output) {

    switch (type) {
      /////////////////////////////////////////
    case drawType::BLOCK_RATE: {
      cv::Scalar color = bgrColor().BLACK();

      if (i.rate() > 1) {
        color = bgrColor().RED();
      } else if (i.rate() == 1) {
        color = bgrColor().GREEN();
      } else {
        color = (0.1 + 0.9 * i.rate()) * bgrColor().BLUE();
      }

      output.setTo(color);
      cv::putText(output, std::to_string(b.getIdSelf()), cv::Point(0, 16),
                  cv::FONT_HERSHEY_PLAIN, 0.8, cv::Scalar(255, 255, 255), 1);
    } break;

      /////////////////////////////////////////
    case drawType::VIEWPORT_RATE: {
      cv::Scalar color = m_viewportRate * bgrColor().BLUE();
      output.setTo(color);
      cv::putText(output, std::to_string(b.getIdSelf()), cv::Point(0, 16),
                  cv::FONT_HERSHEY_PLAIN, 0.8, cv::Scalar(255, 255, 255), 1);

    } break;

      /////////////////////////////////////////
    case drawType::PREDICTION: {
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
      output.setTo(color);
      cv::putText(output, std::to_string(i.decodingOrder), cv::Point(0, 16),
                  cv::FONT_HERSHEY_PLAIN, 0.8, cv::Scalar(0, 0, 0), 1);
    } break;

      /////////////////////////////////////////
    case drawType::PREDICTION_CAT: {
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
      output.setTo(color);
      cv::putText(output, std::to_string(i.decodingOrder), cv::Point(0, 16),
                  cv::FONT_HERSHEY_PLAIN, 0.8, cv::Scalar(255, 255, 255), 1);
    } break;

    case drawType::SOURCE: {
      output *= 2;
    } break;
    }
}

void InteractiveDecoder::updateF1() {
    if (m_showSource) {
      m_decoderCtrl.loadGroundTruthFrame();

      if (decoderCtrl().analyzer().hasGroundTruth()) {
        m_erpImage =
            decoderCtrl().analyzer().GroundTruth->getCurrentFrame() / 2;
        drawBlocks(drawType::SOURCE);
      }

    } else {
      VTM_OPENCV_Utility::toBGRMat(getDecoder()->getDecodedBuf(), m_erpImage);
    }
}

void InteractiveDecoder::updateF2() { drawBlocks(drawType::BLOCK_RATE); }

void InteractiveDecoder::updateF3() {
    m_viewportRate =
        icov::video_layer::VideoMetrics::get_const_instance().viewportRate();

    drawBlocks(drawType::VIEWPORT_RATE);
}

void InteractiveDecoder::updateF4() { drawBlocks(drawType::PREDICTION); }

void InteractiveDecoder::updateF5() { drawBlocks(drawType::PREDICTION_CAT); }

void InteractiveDecoder::updateF6() {
    m_decoderCtrl.loadGroundTruthFrame();
    if (decoderCtrl().analyzer().hasGroundTruth()) {
      VTM_OPENCV_Utility::toBGRMat(getDecoder()->getDecodedBuf(), m_erpImage);
      m_erpImage =
          decoderCtrl().analyzer().GroundTruth->getCurrentFrame() - m_erpImage;
    }
}

bool InteractiveDecoder::processKey(char key)
{
    InteractiveModeBase::processKey(key);
    //    if (!InteractiveModeBase::processKey(key))
    //        return false;

    angle_t pov_x = 0;
    angle_t pov_y = 0;

    // TODO set actual delay based on video framerate
    setWait_delay(30);

    m_decoderCtrl.loadGroundTruthFrame();

    if (!m_requests.empty()) {
      pov_x = m_requests.front().viewpoint.angleX;
      pov_y = m_requests.front().viewpoint.angleY;
      // frame = m_requests.front().frameID;
      frame =
          (frame + 1) % m_decoderCtrl.getDecoder()->icovFile()->getFrameCount();
      m_requests.pop();

      setPOV_DEG(pov_x, pov_y);

      if (m_requests.empty()) {
        // write values for the RD curves
        ICOV_INFO << "ERP RD " << m_erp_rate_sum / m_req_count << ","
                  << m_erp_psnr_sum / m_req_count;
        ICOV_INFO << "DISP RD " << m_vp_rate_sum / m_req_count << ","
                  << m_vp_psnr_sum / m_req_count;

        getchar();
      }

        setWait_delay(1);
    }

    pov_x = getPov_x_DEG();
    pov_y = getPov_y_DEG();

    // ICOV_TRACE << "POV = " << pov_x << ", " << pov_y;

    // decode
    bool updated = getDecoder()->setRequest(
        request_t(DEG2RAD * pov_x, DEG2RAD * pov_y, frame));

    m_viewportMse = -1;

    if (updated && m_decoderCtrl.analyzer().hasCSV() &&
        m_decoderCtrl.analyzer().hasGroundTruth()) {
        VTM_OPENCV_Utility::toBGRMat(getDecoder()->getDecodedBuf(), m_erpImage);

        this->get360viewer()->loadEquirectangularImage(
            m_decoderCtrl.analyzer().GroundTruth->getCurrentFrame());
        this->get360viewer()->getImage_BGR(m_sourceViewport);

        this->get360viewer()->loadEquirectangularImage(m_erpImage);
        this->get360viewer()->getImage_BGR(m_viewportImage);

        m_viewportMse =
            VTM_OPENCV_Utility::mse(m_sourceViewport, m_viewportImage);

        float vp_psnr = icov::maths::psnr(m_viewportMse);
        float vp_rate =
            static_cast<double>(
                m_decoderCtrl.analyzer().CsvOutput->m_viewportEncodedSize) /
            m_viewportImage.total();

        m_decoderCtrl.analyzer().CsvOutput->m_viewportFile.setColumnValue(
            "display pixel area", m_viewportImage.total()); // 3
        m_decoderCtrl.analyzer().CsvOutput->m_viewportFile.setColumnValue(
            "bits per displayed pixel",
            vp_rate); // 7
        m_decoderCtrl.analyzer().CsvOutput->m_viewportFile.setColumnValue(
            "mse (display)", m_viewportMse); // 9
        m_decoderCtrl.analyzer().CsvOutput->m_viewportFile.setColumnValue(
            "psnr (display)", vp_psnr); // 11

        m_vp_rate_sum += vp_rate;
        m_vp_psnr_sum += vp_psnr;

        m_erp_rate_sum += std::stof(
            m_decoderCtrl.analyzer().CsvOutput->m_viewportFile.getColumnValue(
                "bits per decoded pixel"));

        m_erp_psnr_sum += std::stof(
            m_decoderCtrl.analyzer().CsvOutput->m_viewportFile.getColumnValue(
                "psnr (erp)"));

        ++m_req_count;

        m_decoderCtrl.analyzer()
            .CsvOutput->m_viewportFile.writeAndClearValues();

        std::cout << "*******************************\n"
                  << m_viewportMse << std::endl;

        cv::imshow("source", m_sourceViewport);
        cv::imshow("decoded", m_viewportImage);
    }

    if (key == (char)config().custom_keys.at("show_source"))
        m_showSource = !m_showSource;

    if (key == 'f')
        frame = (frame + 1) %
                m_decoderCtrl.getDecoder()->icovFile()->getFrameCount();

    cv::imwrite("./decode.jpg", this->m_erpImage);

    return true;
}
