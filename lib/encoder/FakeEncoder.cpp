#include "FakeEncoder.h"
#include "CommonLib/Unit.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include <boost/filesystem.hpp>
#include <cmath>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace fs = boost::filesystem;

using namespace std;
using namespace icov;

namespace icov {

namespace encoder {

FakeEncoder::FakeEncoder(std::ofstream &output) : m_output(output) {}

FakeEncoder::~FakeEncoder() { std::cout << "destroy FakeEncoder" << std::endl; }

void FakeEncoder::encodeBlockWithPrediction(
    const omnidirectional_layer::Block &b,
    omnidirectional_layer::IBlockPredictionManager::prediction_t prediction) {
  cout << "---- prediction " << prediction << std::endl;

  const omnidirectional_layer::IPicture &p = *(b.parent());
  int bw = (int)(b.w() * p.width());
  int bh = (int)(b.h() * p.height());
  int bx = (int)(p.u(p.angleX(b.u())) * p.width());
  int by = (int)(p.v(p.angleY(b.v())) * p.height());
  auto rect_block = cv::Rect(bx, by, bw, bh);

  cv::Scalar color;

  switch (static_cast<omnidirectional_layer::BlockPrediction::predType>(
      prediction)) {
  case omnidirectional_layer::BlockPrediction::NONE:
    color = cv::Scalar(0, 255, 0);
    break;
  case omnidirectional_layer::BlockPrediction::RIGHT:
    color = cv::Scalar(255, 0, 0);
    break;
  case omnidirectional_layer::BlockPrediction::LEFT:
    color = cv::Scalar(0, 0, 255);
    break;
  case omnidirectional_layer::BlockPrediction::TOP:
    color = cv::Scalar(0, 255, 255);
    break;
  case omnidirectional_layer::BlockPrediction::BOTTOM:
    color = cv::Scalar(255, 0, 255);
    break;
  case omnidirectional_layer::BlockPrediction::TOP_RIGHT:
    color = cv::Scalar(255, 255, 0);
    break;
  case omnidirectional_layer::BlockPrediction::TOP_LEFT:
    color = cv::Scalar(127, 255, 64);
    break;
  case omnidirectional_layer::BlockPrediction::BOTTOM_RIGHT:
    color = cv::Scalar(64, 255, 127);
    break;
  case omnidirectional_layer::BlockPrediction::BOTTOM_LEFT:
    color = cv::Scalar(65, 124, 64);
    break;
  default:
    ICOV_THROW_LOGICAL_EXCEPTION("Bad value");
  }

  cv::Mat pixels = m_frame(rect_block).clone();
  cv::add(0.5 * pixels, 0.5 * color, pixels);

  m_output.write((char *)pixels.data, rect_block.area() * pixels.elemSize());
}

void FakeEncoder::processInput(const IInputMedia<cv::Mat> &input) {

  m_frame = input.getCurrentFrame().clone();
  int values[] = {m_frame.rows, m_frame.cols, m_frame.type(),
                  m_frame.channels()};
  m_output.write(reinterpret_cast<const char *>(&values), sizeof(values));
}

std::vector<omnidirectional_layer::IBlockPredictionManager::prediction_t>
FakeEncoder::getAvailablePredictions(
    const omnidirectional_layer::Block &b, bool access_block,
    const omnidirectional_layer::IBlockPredictionManager &block_prediction) {
  auto v = omnidirectional_layer::BlockPrediction::getAvailablePredictions();
  return std::vector<omnidirectional_layer::IBlockPredictionManager::prediction_t>(v.begin(), v.end());
}
} // namespace encoder
} // namespace icov
