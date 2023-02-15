#include "Block.h"
#include "helpers.h"
#include "omnidirectional_layer/interface/IPicture.h"

const IPicture *Block::parent() const {
  assert(m_parent != nullptr);
  return m_parent;
}

void Block::setId(NeighborDirection index, size_t id) {
  assert(index >= NeighborDirection::SELF && index < NeighborDirection::COUNT);
  uid[index] = id;
}

/// \brief Block::getId
/// \param index
/// \return 0 if not exist, id else
size_t Block::getId(NeighborDirection index) const {
  if (index < NeighborDirection::SELF || index >= NeighborDirection::COUNT)
    return 0;
  return uid[index];
}

size_t Block::getIdSelf() const { return getId(NeighborDirection::SELF); }

const point3d &Block::getCartesianCoord(uint16_t i) const {
  assert(i >= 0 && i < 5);
  return cartesian_coords[i];
}

float Block::u() const { return m_u; }

float Block::v() const { return m_v; }

float Block::w() const { return m_w; }

float Block::h() const { return m_h; }

float Block::center_u() const { return u() + 0.5f * w(); }

float Block::center_v() const { return v() + 0.5f * h(); }

bool Block::isAccessBlock() const { return m_isAccessBlock; }

void Block::setIsAccessBlock(bool newIsAccessBlock) {
  m_isAccessBlock = newIsAccessBlock;
}

Block::Block(size_t id, IPicture *parent, float pu, float pv, float pw,
             float ph)
    : m_parent(parent), m_u(pu), m_v(pv), m_w(pw), m_h(ph) {
  icov::helpers::assert::check(parent != nullptr,
                               "Parent of block is not null");
  icov::helpers::assert::check(id > 0, "ID of block is > 0");

  m_isAccessBlock = false;
  std::fill(uid, uid + NeighborDirection::COUNT, 0);
  setId(NeighborDirection::SELF, id);

  float phi = parent->angleX(center_u());
  float theta = parent->angleY(center_v());
  point3d cart_coord(0, 0, 0);

  bg::transform(angle_radian(phi, theta), cart_coord);
  cartesian_coords[0] = cart_coord;

  phi = parent->angleX(u());
  theta = parent->angleY(v());
  bg::transform(angle_radian(phi, theta), cart_coord);
  cartesian_coords[1] = cart_coord;

  phi = parent->angleX(u() + w());
  bg::transform(angle_radian(phi, theta), cart_coord);
  cartesian_coords[2] = cart_coord;

  theta = parent->angleY(v() + h());
  bg::transform(angle_radian(phi, theta), cart_coord);
  cartesian_coords[3] = cart_coord;

  phi = parent->angleX(u());
  bg::transform(angle_radian(phi, theta), cart_coord);
  cartesian_coords[4] = cart_coord;
}

Block::~Block() { m_parent = nullptr; }
