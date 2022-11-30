#include "InputMedia.h"
#include "common.h"
#include "helpers.h"
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

namespace icov {
namespace encoder {
InputImage::InputImage(const fs::path &path) : icov::IImage<cv::Mat>(path) {
  ICOV_LOG(debug) << "open image " << m_path << std::endl;
  m_frameData = cv::imread(m_path.string(), -1);
  ICOV_ASSERT_MSG(m_frameData.total() > 0, "Empty image");
  m_width = m_frameData.cols;
  m_height = m_frameData.rows;
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

InputVideo::InputVideo(const fs::path &path) : icov::IVideo<cv::Mat>(path) {
  std::cout << "open video " << m_path << std::endl;
}

void InputVideo::setFrame(int frame) {
  m_frameData = 0; /*image = 1 frame only*/
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

  ICOV_ASSERT_MSG(media != nullptr, "Format is not supported: " + ext);
}

} // namespace encoder
} // namespace icov
