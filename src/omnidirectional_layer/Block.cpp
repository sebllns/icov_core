#include "Block.h"

const IPicture *Block::parent() const
{
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

Block::Block(size_t id, IPicture *parent, float pu, float pv, float pw, float ph):
    m_parent(parent),
    m_u(pu),
    m_v(pv),
    m_w(pw),
    m_h(ph)
{
    m_isAccessBlock = false;
    std::fill(uid, uid+NeighborIndex::COUNT, 0);
    setId(NeighborIndex::THIS, id);
}
