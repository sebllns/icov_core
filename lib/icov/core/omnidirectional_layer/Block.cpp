#include "Block.h"

#include "icov_assert.h"
#include "icov_stacktrace.h"
#include "omnidirectional_layer/interface/IPicture.h"
#include <sys/types.h>

namespace icov {
namespace omnidirectional_layer {

Block::Block(Block::id_t id, IPicture *parent, float pu, float pv, float pw,
             float ph)
    : m_parent(parent), m_u(pu), m_v(pv), m_w(pw), m_h(ph), m_hidden(false) {
  icov::assert::not_null(parent,
                         ICOV_INDICATOR_MSG("Parent of block is not null"));
  icov::assert::check(id > 0, ICOV_INDICATOR_MSG("ID of block is > 0"));

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

const IPicture *Block::parent() const {
  assert(m_parent != nullptr);
  return m_parent;
}

bool Block::hidden() const { return m_hidden; }

void Block::setHidden(bool newHidden) { m_hidden = newHidden; }

void Block::setId(NeighborDirection index, Block::id_t id) {
  assert(index >= NeighborDirection::SELF && index < NeighborDirection::COUNT);
  uid[index] = id;
}

/// \brief Block::getId
/// \param index
/// \return 0 if not exist, id else
Block::id_t Block::getId(NeighborDirection index) const {
  if (index < NeighborDirection::SELF || index >= NeighborDirection::COUNT)
    return 0;
  return uid[index];
}

Block::id_t Block::getIdSelf() const
{
  return getId(NeighborDirection::SELF);
}

// size_t getIndex() const
//{
// }

const point3d &Block::getCartesianCoord(uint16_t i) const {
  assert(i >= 0 && i < 5);
  return cartesian_coords[i];
}

float Block::u() const { return m_u; }

float Block::v() const { return m_v; }

float Block::w() const { return m_w; }

float Block::h() const
{
  return m_h;
}

float Block::area() const
{
  return w() * h();
}

int Block::x() const
{
  return u() * parent()->width();
}

int Block::y() const
{
  return v() * parent()->height();
}

int Block::center_x() const
{
  return center_u() * parent()->width();
}

int Block::center_y() const
{
  return center_v() * parent()->height();
}

int Block::iw() const
{
  return w() * parent()->width();
}

int Block::ih() const
{
  return h() * parent()->height();
}

int Block::iarea() const
{
  return iw() * ih();
}

float Block::center_u() const { return u() + 0.5f * w(); }

float Block::center_v() const { return v() + 0.5f * h(); }

bool Block::isAccessBlock() const { return m_isAccessBlock; }

void Block::setIsAccessBlock(bool newIsAccessBlock) {
  m_isAccessBlock = newIsAccessBlock;
}

} // namespace omnidirectional_layer
} // namespace icov
