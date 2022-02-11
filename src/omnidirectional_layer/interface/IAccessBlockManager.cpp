#include "IAccessBlockManager.h"
#include "omnidirectional_layer/interface/IPicture.h"

bool IAccessBlockManager::isValidBlock(BlockVector::const_iterator it)
{
    return it->parent()->isValidBlock(it);
}
