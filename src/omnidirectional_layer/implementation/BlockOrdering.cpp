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
    ICOV_ASSERT_MSG(id > 0 && id <= m_path_info.size(), "ID value is " << id);
    return m_path_info.begin() + id - 1;
}

std::vector<BlockOrdering::block_path_info>::const_iterator BlockOrdering::getPathInfoById(size_t id) const
{
    ICOV_ASSERT_MSG(id > 0 && id <= m_path_info.size(), "ID value is " << id);
    return m_path_info.begin() + id - 1;
}

Block::NeighborIndex BlockOrdering::findAvailableNeighbor(const Block &current, const IViewportManager &vpm)
{
    auto pic = current.parent();

    for(uint16_t n = Block::RIGHT ; n != Block::COUNT ; n++)
    {
        auto nn = static_cast<Block::NeighborIndex>(n);
        auto n_id = current.getId(nn);


#ifdef ICOV_CORE_DEBUG_TRACE_ENABLED
        icov::DebugTools::debug_stream << "    block " << current.getId(Block::THIS) << ", neighbor " << nn;
#endif

        if(pic->blockExists(n_id) && !isBlockProcessed(n_id))
        {
            auto n_block = pic->getById(n_id);
            auto path_info = getPathInfoById(n_id);
            path_info->visibility_tested = true;
            if(vpm.isBlockVisible(*n_block))
            {
#ifdef ICOV_CORE_DEBUG_TRACE_ENABLED
               icov::DebugTools::debug_stream << ": AVAILABLE!\n";
#endif
                path_info->is_visible = true;
                return nn;
            }

#ifdef ICOV_CORE_DEBUG_TRACE_ENABLED
            icov::DebugTools::debug_stream << ": NOT visible...\n";
#endif
            path_info->is_visible = false;
        }
        else
        {
#ifdef ICOV_CORE_DEBUG_TRACE_ENABLED
            icov::DebugTools::debug_stream << ": NOT available...\n";
#endif
        }
    }

    return Block::COUNT;
}

BlockVector::const_iterator BlockOrdering::getNextBlock(const Block &current, const IViewportManager &vpm)
{
#ifdef ICOV_CORE_DEBUG_TRACE_ENABLED
    icov::DebugTools::debug_stream << "block " << m_elements.size() <<" id = " << current.getId(Block::THIS) << std::endl;
#endif

    auto pic = current.parent();
    auto path_it = getPathInfoById(current.getId(Block::THIS));
    path_it->visibility_tested = true;
    path_it->is_visible = true;

    m_elements.push_back(current.getId(Block::THIS));
    m_cursor.push(current.getId(Block::THIS));

    Block::NeighborIndex available = Block::THIS;

    do
    {
        available = findAvailableNeighbor(*pic->getById(m_cursor.top()), vpm);
        if(available == Block::COUNT) m_cursor.pop();
    }
    while(available == Block::COUNT && !m_cursor.empty());

    if(available == Block::COUNT || m_cursor.empty()) return pic->invalidBlock();

    return pic->getById((pic->getById(m_cursor.top()))->getId(available));
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
