#include "IAccessBlockManager.h"

#include "omnidirectional_layer/interface/IViewportManager.h"
#include "omnidirectional_layer/interface/IPicture.h"


const BlockIteratorVector &IAccessBlockManager::blocks() const
{
    return m_blocks;
}

///TODO: RETEST PREVIOUS FIRST AB + MOVE FUNCTION
BlockVector::const_iterator IAccessBlockManager::findAccessBlock(const IViewportManager &vpm) const
{    
//    std::cout << "find in " << m_blocks.size() << " access blocks\n" << std::endl;
    for(const auto &b : m_blocks)
    {
        if(vpm.isBlockVisible(*b)) return b;
    }
    return m_invalidBlock;
}

bool IAccessBlockManager::isAccessBlock(size_t id) const
{
    auto b = m_blocks.begin();

    while (b!=m_blocks.end()) {
        if ((*b)->getId(Block::SELF) == id) return true;
        ++b;
    }

    return false;
}


