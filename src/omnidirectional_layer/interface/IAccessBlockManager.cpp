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
    for(const auto &b : m_blocks)
    {
        if(vpm.isBlockVisible(*b)) return b;
    }
    return m_invalidBlock;
}


