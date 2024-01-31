#include "GenerateGOPMode.h"
#include "Media.h"
#include "app_common/program/icov_program_options.h"
#include "encoder/InputMedia.h"
#include "icov_exception.h"
#include "icov_log.h"
#include "icov_path.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <fstream>
#include <ios>
#include <memory>
#include <string>

using namespace icov;

GenerateGOPMode::GenerateGOPMode(const options::args_t &args) {
  m_video_path = options::parseInputFile(args);
  m_gop_path = options::parseOutputFile(args);
  m_start_frame = options::parseFrameStart(args);
  m_gop_count = options::parseGOPCount(args);
  m_gop_size = options::parseGOPSize(args);
  m_frame_width = options::parseFrameWidth(args);
}

void GenerateGOPMode::run() {
  std::unique_ptr<encoder::InputMedia> video;
  encoder::instantiateInputMedia(m_video_path, video);
  if (!video || video->getType() != eMediaType::VIDEO) {
    ICOV_THROW_LOGICAL_EXCEPTION(m_video_path.string() +
                                 " is not a valid video path");
  }
  path::try_create_directories(m_gop_path.string());

  m_max_frame_count = m_gop_count * m_gop_size;

  frame_t i = 0;
  frame_t imax = video->getFrameCount();

  while (i < imax && (m_max_frame_count == 0 || i < m_max_frame_count)) {
    frame_t f = i + m_start_frame + 1;
    try {
      video->setFrame(f);

      if (video->getFrameID() == f) {
        ICOV_DEBUG << "Export frame " << f;

        int gop_id = i / m_gop_size;
        fs::path gop_path = m_gop_path / std::to_string(gop_id);
        std::string frame_file = std::to_string(i % m_gop_size) + ".tif";

        if (!fs::is_directory(gop_path)) {
          fs::create_directory(gop_path);
        }

        std::ofstream frame_list(
            gop_path / "frames.txt",
            (i % m_gop_size) == 0 ? (std::ios_base::out)
                                  : (std::ios_base::out | std::ios_base::app));

        frame_list << frame_file << std::endl;

        m_current_frame = video->getCurrentFrame();

        if (m_frame_width > 0)
          cv::resize(m_current_frame, m_current_frame,
                     cv::Size(m_frame_width, m_frame_width / 2), 0, 0,
                     cv::INTER_LANCZOS4);

        cv::imwrite((gop_path / frame_file).string(), m_current_frame);
        imax = video->getFrameCount();
      }

      ++i;
    } catch (const std::exception &e) {
      ICOV_ERROR << "Catched error while exporting frame " << f;
      ICOV_ERROR << e.what();
      ICOV_ERROR << boost::current_exception_diagnostic_information();
    } catch (...) {
      ICOV_ERROR << "Unknown error while exporting frame " << f;
      ICOV_ERROR << boost::current_exception_diagnostic_information();
    }
  }
}

std::string GenerateGOPMode::modeID() { return "make-gops"; }
