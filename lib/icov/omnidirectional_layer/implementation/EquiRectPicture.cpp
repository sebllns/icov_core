#include "EquiRectPicture.h"
#include <cmath>

namespace icov {
namespace omnidirectional_layer {
//*/phi = y
// theta = x
EquiRectPicture::EquiRectPicture(int pw, int ph)
    : IPicture(pw, ph, 0, M_PI / 2, 2 * M_PI, M_PI) {}

void EquiRectPicture::buildConstantSizeBlocks(int pw, int ph) {
  int w = m_width;
  int h = m_height;

  // Calculate the number of columns and rows needed to build the blocks
  block_cols = (int)std::ceil((double)w / pw);
  block_rows = (int)std::ceil((double)h / ph);

  // Calculate the total width and height of the blocks
  m_total_w = block_cols * pw;
  m_total_h = block_rows * ph;

  // Calculate the width and height of each block
  float bw = 1.0 / block_cols;
  float bh = 1.0 / block_rows;

  // Reserve memory for the blocks
  m_icus.reserve(block_cols * block_rows);

  // Build each block
  for (int i = 0; i < block_rows; i++) {
    for (int j = 0; j < block_cols; j++) {

      // Get the ID of the current block
      auto id = getId(i, j);

      // Add the block and set its coordinates and size
      auto b = addBlock(id, j * bw, i * bh, bw, bh);

      // Set the IDs of the neighboring blocks
      b->setId(Block::LEFT, getId(i, j - 1));
      b->setId(Block::RIGHT, getId(i, j + 1));
      b->setId(Block::UP, getId(i - 1, j));
      b->setId(Block::DOWN, getId(i + 1, j));
    }
  }
}

size_t EquiRectPicture::getId(int i, int j) const {
  if (i < 0 || i >= block_rows)
    return 0;

  while (j < 0)
    j += block_cols;
  j %= block_cols;

  return i * block_cols + j + 1;
}

float EquiRectPicture::u(float angle_x) const {
  assert(angle_x >= minAngleX() && angle_x <= maxAngleX());
  return (angle_x - minAngleX()) / m_fov_x;
}

float EquiRectPicture::v(float angle_y) const {
  assert(angle_y >= minAngleY() && angle_y <= maxAngleY());
  return (angle_y - minAngleY()) / m_fov_y;
}

float EquiRectPicture::angleX(float u) const {
  return u * m_fov_x + minAngleX();
}

float EquiRectPicture::angleY(float v) const {
  return v * m_fov_y + minAngleY();
}
} // namespace omnidirectional_layer
} // namespace icov
