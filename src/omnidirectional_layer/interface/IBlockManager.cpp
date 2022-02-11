#include "IBlockManager.h"

#include "omnidirectional_layer/interface/IPicture.h"

void IBlockManager::processBlock(BlockVector::const_iterator b)
{
    uint16_t prediction_mask = 0;

    for(uint16_t k = Block::RIGHT ; k < Block::COUNT ; k++)
    {
        if(isProcessed(b->getId((Block::NeighborIndex)k))) prediction_mask |= (1 << k);
    }

    processBlock(*b, prediction_mask);

    m_elements.insert(b->getId(Block::THIS));
}

bool IBlockManager::isProcessed(size_t id)
{
    return m_elements.count(id) > 0;
}
