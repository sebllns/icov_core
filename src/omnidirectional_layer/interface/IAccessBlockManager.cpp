#include "IAccessBlockManager.h"

#include "omnidirectional_layer/interface/IViewportManager.h"
#include "omnidirectional_layer/interface/IPicture.h"

BlockVector::const_iterator IAccessBlockManager::findAccessBlock(const IViewportManager &vpm) const
{
    for(const auto &b : m_blocks)
    {
        if(vpm.isBlockVisible(*b)) return b;
    }

    //BOOST_THROW_EXCEPTION(std::logic_error("Could not find any access block"));

    return m_invalidBlock;
}


