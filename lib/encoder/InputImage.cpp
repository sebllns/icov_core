#include "InputImage.h"
#include "common.h"
#include "helpers.h"
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

namespace icov {
const cv::Mat &InputImage::cvmat() const { return m_cvmat; }

InputImage::InputImage(const std::string &filename) : icov::IImage(filename) {
  std::cout << "open image " << filename << std::endl;
  fs::path filepath = fs::complete(fs::path(filename));
  ICOV_ASSERT_MSG(fs::is_regular_file(filepath),
                  "File not found: " + filepath.string());
  m_cvmat = cv::imread(filename, -1);
  ICOV_WARNING_MSG(m_cvmat.total() > 0, "Empty image");
  m_width = m_cvmat.cols;
  m_height = m_cvmat.rows;
  m_frameCount = 1;
}

} // namespace icov

void icov::InputImage::setFrame(int frame) {}
