#include "GenerateGOPMode.h"
#include "Media.h"
#include "app_common/program/icov_program_options.h"
#include "encoder/InputMedia.h"
#include "icov_exception.h"
#include "icov_log.h"
#include "icov_path.h"
#include "opencv2/imgcodecs.hpp"
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <memory>
#include <string>

using namespace icov;

GenerateGOPMode::GenerateGOPMode(const options::args_t &args) {
  m_video_path = options::parseInputFile(args);
  m_gop_path = options::parseOutputFile(args);
  m_start_frame = 100;
  m_gop_count = 2;
  m_gop_size = 16;
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

        if (!fs::is_directory(gop_path)) {
          fs::create_directory(gop_path);
        }

        gop_path /= std::to_string(i % m_gop_size) + ".tif";

        cv::imwrite(gop_path.string(), video->getCurrentFrame());
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

std::string GenerateGOPMode::modeID() { return "extract-gop"; }
