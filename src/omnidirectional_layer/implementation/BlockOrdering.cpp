#include "BlockOrdering.h"
#include "omnidirectional_layer/interface/IViewportManager.h"
#include "omnidirectional_layer/interface/IPicture.h"

BlockOrdering::BlockOrdering(size_t blocks_count):
    m_path_info(std::vector<block_path_info>(blocks_count))
{
    m_elements.reserve(blocks_count);
}

std::vector<BlockOrdering::block_path_info>::iterator BlockOrdering::getPathInfoById(size_t id)
{
    assert(id > 0 && id <= m_path_info.size());
    return m_path_info.begin() + id - 1;
}

std::vector<BlockOrdering::block_path_info>::const_iterator BlockOrdering::getPathInfoById(size_t id) const
{
    assert(id > 0 && id <= m_path_info.size());
    return m_path_info.begin() + id - 1;
}

Block::NeighborIndex BlockOrdering::findAvailableNeighbor(const Block &current, const IViewportManager &vpm)
{
    auto pic = current.parent();

    for(uint16_t n = Block::RIGHT ; n != Block::COUNT ; n++)
    {
        auto nn = static_cast<Block::NeighborIndex>(n);
        auto n_id = current.getId(nn);
        if(!isBlockProcessed(n_id) && pic->blockExists(n_id))
        {
            auto n_block = pic->getById(n_id);
            auto path_info = getPathInfoById(nn);
            path_info->visibility_tested = true;
            if(vpm.isBlockVisible(*n_block))
            {
                path_info->is_visible = true;
                return nn;
            }
            path_info->is_visible = false;
        }
    }

    return Block::COUNT;
}

BlockVector::const_iterator BlockOrdering::getNextBlock(const Block &current, const IViewportManager &vpm)
{
    auto pic = current.parent();
    auto path_it = getPathInfoById(current.getId(Block::THIS));
    path_it->is_visible = true;


    m_elements.push_back(current.getId(Block::THIS));
    m_cursor.push(current.getId(Block::THIS));

    Block::NeighborIndex available = Block::THIS;

    do
    {
        available = findAvailableNeighbor(current, vpm);
        if(available == Block::COUNT) m_cursor.pop();
    }
    while(available == Block::COUNT && !m_cursor.empty());


    return pic->invalidBlock();
}


void BlockOrdering::reset()
{
    for(size_t id : m_elements)
    {
        getPathInfoById(id)->reset();
    }
    m_elements.clear();
    m_cursor = std::stack<size_t>();
}


bool BlockOrdering::isBlockProcessed(size_t id) const
{
    auto it = getPathInfoById(id);
    return it->visibility_tested && it->is_visible;
}
