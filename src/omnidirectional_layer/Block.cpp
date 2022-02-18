#include "Block.h"
#include "omnidirectional_layer/interface/IPicture.h"

const IPicture *Block::parent() const
{
    assert(m_parent != nullptr);
    return m_parent;
}

void Block::setId(NeighborIndex index, size_t id)
{
    assert(index >= NeighborIndex::THIS && index < NeighborIndex::COUNT);
    uid[index] = id;
}

size_t Block::getId(NeighborIndex index) const
{
    assert(index >= NeighborIndex::THIS && index < NeighborIndex::COUNT);
    return  uid[index];
}

const point3d &Block::getCartesianCoord(uint16_t i) const
{
    assert(i >= 0 && i < 5);
    return cartesian_coords[i];
}

float Block::u() const
{
    return m_u;
}

float Block::v() const
{
    return m_v;
}

float Block::w() const
{
    return m_w;
}

float Block::h() const
{
    return m_h;
}

float Block::center_u() const
{
    return u() + 0.5f * w();
}

float Block::center_v() const
{
    return v() + 0.5f * h();
}

bool Block::isAccessBlock() const
{
    return m_isAccessBlock;
}

//bool Block::isVisible(const viewport_geometry &vpg) const
//{
//    auto rotated = inverse_YPR(cartesian_coords[0], vpg.pov.x(), vpg.pov.y(), 0);

//    auto roi = vpg.roi_size;

//    if(rotated.x() < 0) return false;

//    switch (vpg.visibility_test_strategy)
//    {
//    case viewport_geometry::NONE:
//        return true;
//    case viewport_geometry::CENTER_WITH_PADDING:
//    {
//        bg::multiply_value(roi, 1.2);//todo auto factor from size
//        return is_visible(rotated, roi);
//    }
//    case viewport_geometry::CENTER_AND_CORNERS:
//    {
//        bool visible = false;

//        //TODO

//        return visible;
//    }
//    }

//    return false;
//}

Block::Block(size_t id, IPicture *parent, float pu, float pv, float pw, float ph):
    m_parent(parent),
    m_u(pu),
    m_v(pv),
    m_w(pw),
    m_h(ph)
{
    icov::helpers::assert::check(parent != nullptr, "Parent of block is not null");
    icov::helpers::assert::check(id > 0, "ID of block is > 0");

    m_isAccessBlock = false;
    std::fill(uid, uid+NeighborIndex::COUNT, 0);
    setId(NeighborIndex::THIS, id);

    float phi = parent->angleX(center_u());
    float theta = parent->angleY(center_v());
    point3d cart_coord(0,0,0);


    bg::transform(angle_radian(phi, theta), cart_coord);
    cartesian_coords[0] = cart_coord;

    phi = parent->angleX(u());
    theta = parent->angleY(v());
    bg::transform(angle_radian(phi, theta), cart_coord);
    cartesian_coords[1] = cart_coord;

    phi = parent->angleX(u()+w());
    bg::transform(angle_radian(phi, theta), cart_coord);
    cartesian_coords[2] = cart_coord;

    theta = parent->angleY(v()+h());
    bg::transform(angle_radian(phi, theta), cart_coord);
    cartesian_coords[3] = cart_coord;

    phi = parent->angleX(u());
    bg::transform(angle_radian(phi, theta), cart_coord);
    cartesian_coords[4] = cart_coord;
}

Block::~Block()
{
    m_parent = nullptr;
}
