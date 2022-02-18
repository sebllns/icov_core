#include "RequestManager.h"

#include "omnidirectional_layer/interface/IPicture.h"
#include "omnidirectional_layer/interface/IViewportManager.h"
#include "omnidirectional_layer/interface/IBlockOrdering.h"
#include "omnidirectional_layer/interface/IBlockProcessor.h"
#include "omnidirectional_layer/interface/IAccessBlockManager.h"

void RequestManager::start()
{
    //TODO CHECK EVERYTHING ASSIGNED

    m_accessBlocks->buildAccessBlocks(*m_picture, *m_viewport);
}

void RequestManager::update(float angle_x, float angle_y)
{
    std::cout << "UPDATE " << angle_x << ", " << angle_y << std::endl;

    //TODO: check angle request ?

    m_ordering->reset();
    m_viewport->viewportGeometry().setPov(angle_radian(angle_x, angle_y));

    auto current_block = m_accessBlocks->findAccessBlock(*m_viewport);

    icov::helpers::assert::check(m_picture->isValidBlock(current_block), "No access block was found in the viewport");

    while(m_picture->isValidBlock(current_block))
    {
        std::cout << "block " << current_block->getId(Block::THIS) << std::endl;
        current_block = m_ordering->getNextBlock(*current_block, *m_viewport);
    }

}

const std::shared_ptr<IAccessBlockManager> &RequestManager::accessBlocks() const
{
    return m_accessBlocks;
}

void RequestManager::setAccessBlocks(const std::shared_ptr<IAccessBlockManager> &newAccessBlocks)
{
    m_accessBlocks = newAccessBlocks;
}

const std::shared_ptr<IBlockOrdering> &RequestManager::ordering() const
{
    return m_ordering;
}

void RequestManager::setOrdering(const std::shared_ptr<IBlockOrdering> &newOrdering)
{
    m_ordering = newOrdering;
}

const std::shared_ptr<IViewportManager> &RequestManager::viewport() const
{
    return m_viewport;
}

void RequestManager::setViewport(const std::shared_ptr<IViewportManager> &newViewport)
{
    m_viewport = newViewport;
}

const std::shared_ptr<IPicture> &RequestManager::picture() const
{
    return m_picture;
}

void RequestManager::setPicture(const std::shared_ptr<IPicture> &newPicture)
{
    m_picture = newPicture;
}

const std::shared_ptr<IBlockProcessor> &RequestManager::blockProcessor() const
{
    return m_blockProcessor;
}

void RequestManager::setBlockProcessor(const std::shared_ptr<IBlockProcessor> &newBlockProcessor)
{
    m_blockProcessor = newBlockProcessor;
}

