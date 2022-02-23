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
    std::cout << "VPG " << vpm.viewportGeometry().pov.x() <<" ; " << vpm.viewportGeometry().pov.y() << std::endl;

    for(const auto &b : m_blocks)
    {
        if(vpm.isBlockVisible(*b)) return b;
    }

    std::cout << "did not found access block" << std::endl;
    return m_invalidBlock;
}


