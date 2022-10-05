#include "FakeEncoder.h"
#include "CommonLib/Unit.h"
#include "omnidirectional_layer/BlockPrediction.h"
#include <boost/filesystem.hpp>
#include <cmath>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace fs = boost::filesystem;

using namespace std;
using namespace icov;

namespace icov {

FakeEncoder::FakeEncoder() { cout << "new encoder" << endl; }

FakeEncoder::~FakeEncoder() {
  cout << "destroy encoder" << endl;
  //  dispose();
}

// void FakeEncoder::openFile(const std::string &filename) {
//  cout << "open file to encode " << filename << std::endl;
//  fs::path filepath = fs::complete(fs::path(filename));

//  ICOV_ASSERT_MSG(fs::is_regular_file(filepath),
//                  "File not found: " + filepath.string());
//  m_img_src = cv::imread(filename, -1);
//}

// void FakeEncoder::dispose() {
//  m_img_src.release();
//  cout << "dispose" << endl;
//}

void FakeEncoder::encode(std::ostream &output, const IPicture &picture,
                         const IAccessBlockManager &access_blocks) {
  ICOV_ASSERT_MSG(m_img_src.total() > 0, "Cannot encode empty image");
  ICOV_ASSERT_MSG((size_t)m_img_src.rows == picture.height() &&
                      (size_t)m_img_src.cols == picture.width(),
                  "Picture is not the right size");

  // output << m_img_src.rows << " " << m_img_src.cols << " " <<
  // m_img_src.type() << " " << m_img_src.channels();

  int values[] = {m_img_src.rows, m_img_src.cols, m_img_src.type(),
                  m_img_src.channels()};
  output.write(reinterpret_cast<const char *>(&values), sizeof(values));

  // encode the blocks
  cout << "encode the blocks" << std::endl;
  for (const auto &b : picture.icus()) {
    encodeBlock(output, b, access_blocks.isAccessBlock(b.getId(Block::SELF)));
  }
}

void FakeEncoder::encodeBlock(std::ostream &output, const Block &b,
                              bool access_block) {
  cout << "-- block " << b.center_u() << ", " << b.center_v() << std::endl;

  // if(access_block) encodeBlockWithPrediction(output, b,
  // BlockPrediction::NONE);

  std::vector<BlockPrediction::predType> pred_values = {
      BlockPrediction::RIGHT,        BlockPrediction::LEFT,
      BlockPrediction::TOP,          BlockPrediction::BOTTOM,
      BlockPrediction::TOP_RIGHT,    BlockPrediction::TOP_LEFT,
      BlockPrediction::BOTTOM_RIGHT, BlockPrediction::BOTTOM_LEFT,
  };

  for (auto pred : pred_values) {
    encodeBlockWithPrediction(output, b, static_cast<uint16_t>(pred));
  }
}

void FakeEncoder::encodeBlockWithPrediction(std::ostream &output,
                                            const Block &b,
                                            uint16_t prediction) {
  cout << "---- prediction " << prediction << std::endl;

  const IPicture &p = *(b.parent());
  int bw = (int)(b.w() * p.width());
  int bh = (int)(b.h() * p.height());
  int bx = (int)(p.u(p.angleX(b.u())) * p.width());
  int by = (int)(p.v(p.angleY(b.v())) * p.height());
  auto rect_block = cv::Rect(bx, by, bw, bh);

  cv::Scalar color;

  switch (static_cast<BlockPrediction::predType>(prediction)) {
  case BlockPrediction::NONE:
    color = cv::Scalar(0, 255, 0);
    break;
  case BlockPrediction::RIGHT:
    color = cv::Scalar(255, 0, 0);
    break;
  case BlockPrediction::LEFT:
    color = cv::Scalar(0, 0, 255);
    break;
  case BlockPrediction::TOP:
    color = cv::Scalar(0, 255, 255);
    break;
  case BlockPrediction::BOTTOM:
    color = cv::Scalar(255, 0, 255);
    break;
  case BlockPrediction::TOP_RIGHT:
    color = cv::Scalar(255, 255, 0);
    break;
  case BlockPrediction::TOP_LEFT:
    color = cv::Scalar(127, 255, 64);
    break;
  case BlockPrediction::BOTTOM_RIGHT:
    color = cv::Scalar(64, 255, 127);
    break;
  case BlockPrediction::BOTTOM_LEFT:
    color = cv::Scalar(65, 124, 64);
    break;
  }

  cv::Mat pixels = m_img_src(rect_block).clone();
  cv::add(0.5 * pixels, 0.5 * color, pixels);

  output.write((char *)pixels.data, rect_block.area() * pixels.elemSize());
}
} // namespace icov
