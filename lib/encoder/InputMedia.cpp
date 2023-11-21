#include "InputMedia.h"
#include "icov_assert.h"
#include "icov_exception.h"
#include "icov_path.h"
#include "opencv2/videoio.hpp"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

namespace icov {
namespace encoder {
InputImage::InputImage(const fs::path &path) : icov::IImage<cv::Mat>(path) {
  ICOV_DEBUG << "open image " << m_path << std::endl;
  m_frameData = cv::imread(m_path.string(), -1);
  ICOV_ASSERT_MSG(m_frameData.total() > 0, "Empty image");
  m_width = m_frameData.cols;
  m_height = m_frameData.rows;
  icov::assert::check(m_width > 0, ICOV_DEBUG_HEADER("Image width > 0"));
  icov::assert::check(m_height > 0, ICOV_DEBUG_HEADER("Image height > 0"));
}

void InputImage::writeInfos(std::ofstream &output, int key, void *param) {

  switch (key) {
  case 0: {
    // HEADER
    const int values[] = {m_frameData.rows, m_frameData.cols,
                          m_frameData.type(), m_frameData.channels()};
    output.write(reinterpret_cast<const char *>(&values), sizeof(values));
    // end header
  } break;
  default:
    ICOV_WARNING_MSG(false, "Unknown key id");
  }
}

InputVideo::InputVideo(const fs::path &path) : icov::IVideo<cv::Mat>(path), m_videoCV(path.string())
{
  std::cout << "open video " << m_path << std::endl;
  icov::assert::check(m_videoCV.isOpened(), ICOV_DEBUG_HEADER("Video width > 0"));

  m_width = m_videoCV.get(cv::CAP_PROP_FRAME_WIDTH);
  icov::assert::check(m_width > 0, ICOV_DEBUG_HEADER("Video width > 0"));

  m_height = m_videoCV.get(cv::CAP_PROP_FRAME_HEIGHT);
  icov::assert::check(m_height > 0, ICOV_DEBUG_HEADER("Video height > 0"));

  m_frameCount = 1;
}

void InputVideo::setFrame(frame_t i) {
  if (i != (getFrameID() + 1)) {
    m_videoCV.set(cv::CAP_PROP_POS_FRAMES, i);
  }
  if (m_videoCV.isOpened() && m_videoCV.read(m_frameData)) {
    m_frameID = i;
    m_frameCount = i + 1;
    std::cout << "FRAME " << m_frameID << "/" << m_frameCount << std::endl;
  }
}

void InputVideo::writeInfos(std::ofstream &output, int key, void *param) {

  switch (key) {
  case 0: {
    // HEADER
    const int values[] = {m_frameData.rows, m_frameData.cols,
                          m_frameData.type(), m_frameData.channels()};
    output.write(reinterpret_cast<const char *>(&values), sizeof(values));
    // end header
  } break;
  default:
    ICOV_WARNING_MSG(false, "Unknown key id");
  }
}

void instantiateInputMedia(const boost::filesystem::path &path,
                           std::unique_ptr<InputMedia> &output) {

  path::assert_file_exists(path.string());

  InputMedia *media = nullptr;
  std::string ext =
      boost::algorithm::to_lower_copy(path.extension().string()).erase(0, 1);

  if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "bmp" ||
      ext == "dib" || ext == "jpe" || ext == "jp2" || ext == "pbm" ||
      ext == "pgm" || ext == "ppm" || ext == "tiff" || ext == "tif") {
    media = new InputImage(path);
  }

  if (ext == "avi" || ext == "mp4" || ext == "mkv" || ext == "mov") {
    media = new InputVideo(path);
  }

  output.reset(media);

  assert::not_null(media, "Format is not supported: " + ext);
}

} // namespace encoder
} // namespace icov
