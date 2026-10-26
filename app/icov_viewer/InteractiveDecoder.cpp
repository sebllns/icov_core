#include "InteractiveDecoder.h"

#include "app_common/program/icov_program_options.h"
#include "app_common/tools/icov_udp.h"
#include "icov_assert.h"
#include "icov_log.h"
#include "icov_math.h"
#include "icov_path.h"
#include "icov_request.h"
#include "opencv2/core.hpp"
#include "opencv2/core/base.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/persistence.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "video_layer/VideoMetrics.h"
#include "video_layer/icov_stream.h"
#include "video_layer/vtm_opencv_utility.h"

#include "opencv2/core/types.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <unistd.h>

InteractiveDecoder::InteractiveDecoder(const path::path_t &icov_file,
                                       const options::args_t &args)
    : InteractiveModeBase(), m_decoderCtrl(icov_file, args),
      m_showSource(false), is_playing(true) {
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
  m_frame = 0;
  is_decoding = true;
  // A still image has nothing to play: avoid decoding it again on every loop
  is_playing = getDecoder()->contentParams().content_framecount > 1;

  m_monitor = cv::Mat(cv::Size(400, 200), CV_8UC3, bgrColor::BLACK());
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

    auto rect_block = VTM_OPENCV_Utility::getBlockRoi(*b);
    auto roi = this->m_fullView(rect_block);

    drawBlock(type, *b, i, roi, rect_block);
  }
}

void InteractiveDecoder::drawBlock(
    drawType type, const Block &b,
    const video_layer::VideoMetrics::block_infos_t &i, cv::Mat &output,
    const cv::Rect &roi) {

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
                cv::FONT_HERSHEY_PLAIN, 0.8, bgrColor::WHITE(), 1);
    cv::rectangle(output,
                  cv::Rect(roi.x + 1, roi.y + 1, roi.width - 1, roi.height - 1),
                  bgrColor::GREEN(), 1);
  } break;

    /////////////////////////////////////////
  case drawType::VIEWPORT_RATE: {
    cv::Scalar color = m_viewportRate * bgrColor().BLUE();
    output.setTo(color);
    cv::rectangle(output, roi, bgrColor::GREEN(), 1);
    cv::putText(output, std::to_string(b.getIdSelf()), cv::Point(0, 16),
                cv::FONT_HERSHEY_PLAIN, 0.8, bgrColor::WHITE(), 1);

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
                cv::FONT_HERSHEY_PLAIN, 0.8, bgrColor::BLACK(), 1);
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
                cv::FONT_HERSHEY_PLAIN, 0.8, bgrColor::BLACK(), 1);
  } break;

    /////////////////////////////////////////
  case drawType::SOURCE: {
    output *= 2;
  } break;

    /////////////////////////////////////////
  case drawType::SOURCE_DIFF: {
    auto s = cv::sum(m_groundTruthDiff(roi));
    bool is_good = (s == s.zeros());
    output.setTo(is_good ? bgrColor::GREEN() : bgrColor::RED());
    cv::putText(output, std::to_string(i.decodingOrder), cv::Point(0, 16),
                cv::FONT_HERSHEY_PLAIN, 0.8, bgrColor::BLACK(), 1);
    if (!is_good) {
      ICOV_ERROR << "Bad decoded block " << b.getIdSelf();
      ICOV_ERROR << "Frame: " << i.frame;
      ICOV_ERROR << "Decoding order: " << i.decodingOrder;
      ICOV_ERROR << "Prediction: " << BlockPrediction::toString(i.prediction);
      ICOV_ERROR << "Rate: " << i.rate();
    }
  } break;

    /////////////////////////////////////////
  case drawType::DECODED: {
    ICOV_DEBUG << roi;
    m_decodedErp(roi).copyTo(output);
  } break;
  }
}

void InteractiveDecoder::updateF1() {
  if (m_showSource) {
    m_decoderCtrl.loadGroundTruthFrame();
    if (decoderCtrl().analyzer().hasGroundTruth()) {
      m_fullView = decoderCtrl().analyzer().GroundTruth->getCurrentFrame() / 2;
      drawBlocks(drawType::SOURCE);
    }
  } else {
    drawBlocks(drawType::DECODED);
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
  if (is_decoding) {
    m_decoderCtrl.loadGroundTruthFrame();
    if (decoderCtrl().analyzer().hasGroundTruth()) {
      m_groundTruthDiff =
          decoderCtrl().analyzer().GroundTruth->getCurrentFrame() -
          m_decodedErp;
      drawBlocks(drawType::SOURCE_DIFF);
    }
  }
}

bool InteractiveDecoder::processKey(char key) {
  InteractiveModeBase::processKey(key);
  //    if (!InteractiveModeBase::processKey(key))
  //        return false;

  angle_t pov_x = 0;
  angle_t pov_y = 0;

  // TODO set actual delay based on video framerate
  setWait_delay(30);

  if (!m_requests.empty()) {
    pov_x = m_requests.front().viewpoint.angleX;
    pov_y = m_requests.front().viewpoint.angleY;
    // frame = m_requests.front().frameID;
    m_frame = m_requests.front().frameID;
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
  m_viewportMse = -1;
  bool updated = getDecoder()->setRequest(
      request_t(DEG2RAD * pov_x, DEG2RAD * pov_y, m_frame));

  if (updated) {

    float vp_psnr = 0;
    float vp_rate = 0;

    m_decoderCtrl.loadGroundTruthFrame();
    m_decoderCtrl.loadSourceFrame();

    VTM_OPENCV_Utility::toBGRMat(getDecoder()->getDecodedBuf(), m_decodedErp);

    if (m_fullView.size() != m_decodedErp.size()) {
      m_fullView = m_decodedErp.clone();
      m_fullView.setTo(bgrColor::BLACK());
    }

    if (is_decoding) {
      this->get360viewer()->loadEquirectangularImage(m_decodedErp);
      this->get360viewer()->getImage_BGR(m_viewportView);
    }

    vp_rate = (float)video_layer::VideoMetrics::get_const_instance()
                  .viewportEncodedBitlength() /
              m_viewportView.total();

    //    cv::imshow("decoded", m_viewportView);

    if (m_decoderCtrl.analyzer().hasGroundTruth()) {
      this->get360viewer()->loadEquirectangularImage(
          m_decoderCtrl.analyzer().GroundTruth->getCurrentFrame());
      this->get360viewer()->getImage_BGR(m_sourceViewport);

      if (is_decoding) {
        m_viewportMse =
            VTM_OPENCV_Utility::mse(m_sourceViewport, m_viewportView);
        //      cv::imshow("ground truth", m_sourceViewport);

        if (m_viewportMse != 0) {
          ICOV_ERROR << "Request " << to_string(getDecoder()->getRequest());
          ICOV_ERROR << "Decoding error detected! "
                     << " MSE should be 0, but MSE=" << m_viewportMse;
          ICOV_ERROR << "Press F6 to detect bad decoded blocks";
          getDecoder()->resetCache();
        }
      } else {
        m_decoderCtrl.analyzer().GroundTruth->getCurrentFrame().copyTo(
            m_decodedErp);
        m_sourceViewport.copyTo(m_viewportView);
      }
    }

    if (m_decoderCtrl.analyzer().hasSource()) {
      this->get360viewer()->loadEquirectangularImage(
          m_decoderCtrl.analyzer().Source->getCurrentFrame());
      this->get360viewer()->getImage_BGR(m_sourceViewport);
      m_viewportMse = VTM_OPENCV_Utility::mse(m_sourceViewport, m_viewportView);
      vp_psnr = icov::maths::psnr(m_viewportMse);
      //      cv::imshow("source", m_sourceViewport);
    }

    try {
      icov::udp::send_udp_message(std::to_string(vp_rate) + " " +
                                      std::to_string(vp_psnr),
                                  "127.0.0.1", 5005);
    } catch (...) {
    }
  }

  //    if (updated && m_decoderCtrl.analyzer().hasCSV() &&
  //        m_decoderCtrl.analyzer().hasGroundTruth()) {
  //        VTM_OPENCV_Utility::toBGRMat(getDecoder()->getDecodedBuf(),
  //        m_fullView);

  //        this->get360viewer()->loadEquirectangularImage(
  //            m_decoderCtrl.analyzer().GroundTruth->getCurrentFrame());
  //        this->get360viewer()->getImage_BGR(m_sourceViewport);

  //        this->get360viewer()->loadEquirectangularImage(m_fullView);
  //        this->get360viewer()->getImage_BGR(m_viewportView);

  //        m_viewportMse =
  //            VTM_OPENCV_Utility::mse(m_sourceViewport, m_viewportView);

  //        float vp_psnr = icov::maths::psnr(m_viewportMse);
  //        float vp_rate =
  //            static_cast<double>(
  //                m_decoderCtrl.analyzer().CsvOutput->m_viewportEncodedSize)
  //                /
  //            m_viewportView.total();

  //        m_decoderCtrl.analyzer().CsvOutput->m_viewportFile.setColumnValue(
  //            "display pixel area", m_viewportView.total()); // 3
  //        m_decoderCtrl.analyzer().CsvOutput->m_viewportFile.setColumnValue(
  //            "bits per displayed pixel",
  //            vp_rate); // 7
  //        m_decoderCtrl.analyzer().CsvOutput->m_viewportFile.setColumnValue(
  //            "mse (display)", m_viewportMse); // 9
  //        m_decoderCtrl.analyzer().CsvOutput->m_viewportFile.setColumnValue(
  //            "psnr (display)", vp_psnr); // 11

  //        m_vp_rate_sum += vp_rate;
  //        m_vp_psnr_sum += vp_psnr;

  //        m_erp_rate_sum += std::stof(
  //            m_decoderCtrl.analyzer().CsvOutput->m_viewportFile.getColumnValue(
  //                "bits per decoded pixel"));

  //        m_erp_psnr_sum += std::stof(
  //            m_decoderCtrl.analyzer().CsvOutput->m_viewportFile.getColumnValue(
  //                "psnr (erp)"));

  //        ++m_req_count;

  //        m_decoderCtrl.analyzer()
  //            .CsvOutput->m_viewportFile.writeAndClearValues();

  //        std::cout << "*******************************\n"
  //                  << m_viewportMse << std::endl;

  //        cv::imshow("source", m_sourceViewport);
  //        cv::imshow("decoded", m_viewportView);
  //    }

  if (key == (char)config().custom_keys.at("show_source"))
    m_showSource = !m_showSource;

  m_monitor.setTo(bgrColor::BLACK());

  int hline = 15;
  int yline = 10;
  std::stringstream info_txt;
  info_txt << "Video: ";
  info_txt << (is_playing ? "playing" : "paused");
  info_txt << " ; frame " << m_frame;
  cv::putText(m_monitor, info_txt.str(), cv::Point(10, yline),
              cv::FONT_HERSHEY_PLAIN, 1, bgrColor::PASTEL_GREEN(), 1);

  yline += hline;
  info_txt = std::stringstream();
  info_txt << "Lon.: " << pov_x + 180 << ", Lat.: " << pov_y;
  cv::putText(m_monitor, info_txt.str(), cv::Point(10, yline),
              cv::FONT_HERSHEY_PLAIN, 1, bgrColor::PASTEL_GREEN(), 1);

  yline += hline;
  info_txt = std::stringstream();
  info_txt << "ICOV Decoder: " << (is_decoding ? "ON" : "OFF");
  cv::putText(m_monitor, info_txt.str(), cv::Point(10, yline),
              cv::FONT_HERSHEY_PLAIN, 1, bgrColor::PASTEL_GREEN(), 1);

  cv::imshow("ICOV Monitor", m_monitor);

  if (key == 'p')
    is_playing = !is_playing;

  if (is_playing || key == 'f')
    ++m_frame;

  if (key == 'c') {
    is_decoding = !is_decoding;
    m_decoderCtrl.getDecoder()->resetCache();
    m_decoderCtrl.getDecoder()->blockDecoder()->switchBypassDecodingFlag();
  }

  return true;
}
