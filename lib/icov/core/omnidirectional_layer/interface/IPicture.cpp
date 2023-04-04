#include "IPicture.h"
#include "omnidirectional_layer/Block.h"

RequestManager *IPicture::requestManager() const {
  assert(m_requestManager != nullptr);
  return m_requestManager;
}

void IPicture::setRequestManager(RequestManager *newRequestManager) {
  m_requestManager = newRequestManager;
  assert(m_requestManager != nullptr);
}

void IPicture::releaseRequestManager() { m_requestManager = nullptr; }

IPicture::IPicture(size_t w, size_t h, float center_angle_x,
                   float center_angle_y, float fov_x, float fov_y)
    : m_width(w), m_height(h), m_total_w(w), m_total_h(h),
      m_center_angle_x(center_angle_x), m_center_angle_y(center_angle_y),
      m_fov_x(fov_x), m_fov_y(fov_y) {}

const BlockVector &IPicture::icus() const { return m_icus; }

bool IPicture::blockExists(Block::id_t id) const {
  return id > 0 && id <= m_icus.size();
}

BlockVector::iterator IPicture::getById(Block::id_t id) {
  if (!blockExists(id))
    return m_icus.end();
  return m_icus.begin() + id - 1;
}

BlockVector::const_iterator IPicture::getById(Block::id_t id) const {
  if (!blockExists(id))
    return m_icus.end();
  return m_icus.begin() + id - 1;
}

BlockVector::const_iterator IPicture::getBlock(int i, int j) const {
  return getById(getId(i, j));
}

bool IPicture::isValidBlock(BlockVector::const_iterator it) const {
  return it != invalidBlock();
}

void IPicture::setAccessBlockFlag(Block::id_t id) {
  getById(id)->setIsAccessBlock(true);
}

BlockVector::const_iterator IPicture::invalidBlock() const {
  return m_icus.end();
}

BlockVector::iterator IPicture::addBlock(Block::id_t id, float pu, float pv,
                                         float pw, float ph) {
  m_icus.push_back(Block(id, this, pu, pv, pw, ph));
  return std::prev(m_icus.end());
}

size_t IPicture::blockCount() const { return m_icus.size(); }

size_t IPicture::width() const { return m_width; }

size_t IPicture::height() const { return m_height; }

size_t IPicture::total_w() const { return m_total_w; }

size_t IPicture::total_h() const { return m_total_h; }

float IPicture::center_angle_x() const { return m_center_angle_x; }

float IPicture::center_angle_y() const { return m_center_angle_y; }

float IPicture::fov_x() const { return m_fov_x; }

float IPicture::fov_y() const { return m_fov_y; }

point3d IPicture::getCartesianCoords(Block::id_t id) const {
  angle_radian p_spher(angleX(getById(id)->center_u()),
                       angleY(getById(id)->center_v()));
  point3d p_cart(0.0f, 0.0f, 0.0f);
  bg::transform(p_spher, p_cart);
  return p_cart;
}

void IPicture::getPixelCoords(float u, float v, int &i, int &j) const {
  i = this->row(u);
  j = this->col(v);
}

int IPicture::col(float u) const {
  return static_cast<int>(std::floor(u * static_cast<float>(m_total_w)));
}

int IPicture::row(float v) const {
  return static_cast<int>(std::floor(v * static_cast<float>(m_total_h)));
}

float IPicture::u(int i) const {
  return static_cast<float>(i) / static_cast<float>(m_total_w);
}

float IPicture::v(int j) const {
  return static_cast<float>(j) / static_cast<float>(m_total_h);
}

float IPicture::minAngleX() const { return m_center_angle_x - 0.5f * m_fov_x; }

float IPicture::maxAngleX() const { return m_center_angle_x + 0.5f * m_fov_x; }

float IPicture::minAngleY() const { return m_center_angle_y - 0.5f * m_fov_y; }

float IPicture::maxAngleY() const { return m_center_angle_y + 0.5f * m_fov_y; }
