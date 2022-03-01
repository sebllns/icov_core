#include "Encoder.h"

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

Encoder::Encoder()
{
  std::cout << "new encoder" << std::endl;
}

Encoder::~Encoder()
{
  std::cout << "destroy encoder" << std::endl;
}

void Encoder::openFile(const std::string &filename)
{
    fs::path filepath = fs::complete(fs::path(filename));

    ICOV_ASSERT_MSG(fs::is_regular_file(filepath), "File not found: " + filepath.string());

    m_img_src = cv::imread(filename, -1);
}

void Encoder::dispose()
{
}

void Encoder::encode(std::ostream &ostr)
{
    ICOV_ASSERT_MSG(m_img_src.total() > 0, "Cannot encode empty image");
    ostr << m_img_src.rows << m_img_src.cols;
    //block size as parameter
}

void Encoder::encodeBlock(const Block &b)
{
}

void Encoder::encodeBlockWithPrediction(const Block &b, uint16_t prediction)
{
}
