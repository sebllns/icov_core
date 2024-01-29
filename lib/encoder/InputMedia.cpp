#include "InputMedia.h"
#include "icov_assert.h"
#include "icov_exception.h"
#include "icov_path.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/videoio.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/directory.hpp>
#include <boost/filesystem/path.hpp>
#include <fstream>
#include <limits>
#include <memory>
namespace fs = boost::filesystem;

namespace icov {
namespace encoder {
InputImage::InputImage(const fs::path &path) : icov::IImage<cv::Mat>(path) {
  ICOV_DEBUG << "Open image " << m_path << std::endl;
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
  m_frameID = 0;
}

void InputVideo::setFrame(frame_t i) {
  // doing this because sometimes the video does not return correct value of
  // number of frames so we have to read frame by frame
  if (i != (getFrameID() + 1)) {
    m_videoCV.set(cv::CAP_PROP_POS_FRAMES, i);
  }
  if (m_videoCV.isOpened() && m_videoCV.read(m_frameData) &&
      !m_frameData.empty()) {
    m_frameID = i;
    m_frameCount = i + 1;
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

  if (fs::is_directory(path)) {
    output = std::make_unique<InputGop>(path);
    return;
  }

  path::assert_file_exists(path.string());

  InputMedia *media = nullptr;
  std::string ext =
      boost::algorithm::to_lower_copy(path.extension().string()).erase(0, 1);

  if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "bmp" ||
      ext == "dib" || ext == "jpe" || ext == "jp2" || ext == "pbm" ||
      ext == "pgm" || ext == "ppm" || ext == "tiff" || ext == "tif") {
    media = new InputImage(path);
  }

  if (ext == "avi" || ext == "mp4" || ext == "mkv" || ext == "mov" ||
      ext == "webm") {
    media = new InputVideo(path);
  }

  output.reset(media);

  assert::not_null(media, ICOV_DEBUG_HEADER("Format is not supported: " + ext));
}

InputGop::InputGop(const boost::filesystem::path &path)
    : IInputMedia<cv::Mat>(path, (int)eMediaType::VIDEO) {
  ICOV_DEBUG << "Open GOP " << path;
  std::ifstream frame_txt((getPath() / "frames.txt").string());

  for (std::string line; getline(frame_txt, line);) {
    if (fs::is_regular_file(getPath() / line)) {
      m_frames.push_back(line);
    }
  }

  m_frameCount = m_frames.size();

  m_frameID = std::numeric_limits<frame_t>::max();
  setFrame(0);
  m_width = m_frameData.cols;
  m_height = m_frameData.rows;
  icov::assert::check(m_width > 0, ICOV_DEBUG_HEADER("Image width > 0"));
  icov::assert::check(m_height > 0, ICOV_DEBUG_HEADER("Image height > 0"));
}

void InputGop::setFrame(frame_t i) {
  if (i == getFrameID())
    return;

  if (i >= m_frameCount)
    ICOV_THROW_INVALID_ARGUMENT("Cannot read frame " + std::to_string(i));

  m_frameID = i;

  bool first = m_frameData.empty();

  fs::path p(getPath() / m_frames[i]);

  if (!fs::is_regular_file(p)) {
    ICOV_THROW_LOGICAL_EXCEPTION("File not found " + p.string());
  }

  m_frameData = cv::imread(p.string());

  if (!first) {
    icov::assert::check((int)m_width == m_frameData.cols,
                        ICOV_DEBUG_HEADER("Bad frame width"));
    icov::assert::check((int)m_height == m_frameData.rows,
                        ICOV_DEBUG_HEADER("Bad frame height"));
  }
}

void InputGop::writeInfos(std::ofstream &output, int key, void *param) {}

} // namespace encoder
} // namespace icov
