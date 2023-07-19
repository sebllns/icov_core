#include "RequestManager.h"
#include "common.h"
#include "omnidirectional_layer/interface/IAccessBlockManager.h"
#include "omnidirectional_layer/interface/IBlockOrdering.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include "omnidirectional_layer/interface/IBlockProcessor.h"
#include "omnidirectional_layer/interface/IPicture.h"
#include "omnidirectional_layer/interface/IViewportManager.h"

namespace icov {
namespace omnidirectional_layer {

const std::shared_ptr<IBlockPredictionManager> &
RequestManager::blockPrediction() const {
  return m_blockPrediction;
}

void RequestManager::setBlockPrediction(
    const std::shared_ptr<IBlockPredictionManager> &newBlockPrediction) {
  m_blockPrediction = newBlockPrediction;
}

void RequestManager::start() {
  icov::assert::not_null(m_blockProcessor.get(),
                         "Block processor is not allocated");
  icov::assert::not_null(m_accessBlocks.get(),
                         "Access block manager is not allocated");
  m_accessBlocks->buildAccessBlocks(*m_picture, *m_viewport);
}

void RequestManager::update(float angle_x, float angle_y) {
#if ICOV_ASSERTION_ENABLED
  icov::assert::not_null(m_ordering.get(), "Ordering manager cannot be null");
  icov::assert::not_null(m_viewport.get(), "Viewport manager cannot be null");
  icov::assert::not_null(m_accessBlocks.get(),
                         "Access blocks manager manager cannot be null");

  icov::assert::not_null(m_blockPrediction.get(),
                         "Predictions manager manager cannot be null");
#endif

  m_ordering->reset();
  m_viewport->viewportGeometry().setPov(angle_radian(angle_x, angle_y));

  auto current_block = m_accessBlocks->findAccessBlock(*m_viewport);

  bool is_access_block = true;

#ifdef ICOV_ASSERTION_ENABLED
  icov::assert::check(m_picture->isValidBlock(current_block),
                      "No access block was found in the viewport");
#endif

  if (m_blockProcessor)
    m_blockProcessor->startProcess();
  while (m_picture->isValidBlock(current_block)) {
    if (m_blockProcessor) {
      try {
        m_blockProcessor->processBlock(
            *current_block, is_access_block ? 0
                                            : m_blockPrediction->getPrediction(
                                                  *current_block, *m_ordering));
      } catch (...) {
        throw;
      }
    }

    current_block = m_ordering->getNextBlock(*current_block, *m_viewport);
    is_access_block = false;
  }
  if (m_blockProcessor)
    m_blockProcessor->endProcess();
}

const std::shared_ptr<IAccessBlockManager> &
RequestManager::accessBlocks() const {
  return m_accessBlocks;
}

void RequestManager::setAccessBlocks(
    const std::shared_ptr<IAccessBlockManager> &newAccessBlocks) {
  m_accessBlocks = newAccessBlocks;
}

const std::shared_ptr<IBlockOrdering> &RequestManager::ordering() const {
  return m_ordering;
}

void RequestManager::setOrdering(
    const std::shared_ptr<IBlockOrdering> &newOrdering) {
  m_ordering = newOrdering;
}

const std::shared_ptr<IViewportManager> &RequestManager::viewport() const {
  return m_viewport;
}

void RequestManager::setViewport(
    const std::shared_ptr<IViewportManager> &newViewport) {
  m_viewport = newViewport;
}

const std::shared_ptr<IPicture> &RequestManager::picture() const {
  return m_picture;
}

void RequestManager::setPicture(const std::shared_ptr<IPicture> &newPicture) {
  m_picture = newPicture;
}

const std::shared_ptr<IBlockProcessor> &RequestManager::blockProcessor() const {
  return m_blockProcessor;
}

void RequestManager::setBlockProcessor(
    const std::shared_ptr<IBlockProcessor> &newBlockProcessor) {
  m_blockProcessor = newBlockProcessor;
}
} // namespace omnidirectional_layer
} // namespace icov
