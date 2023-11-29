#include "RequestManager.h"
#include "icov_assert.h"
#include "icov_log.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/icov_geometry.h"
#include "omnidirectional_layer/interface/IAccessBlockManager.h"
#include "omnidirectional_layer/interface/IBlockOrdering.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include "omnidirectional_layer/interface/IBlockProcessor.h"
#include "omnidirectional_layer/interface/IPicture.h"
#include "omnidirectional_layer/interface/IViewportManager.h"
#include <limits>

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

const request_t &RequestManager::request() const
{
  return m_request;
}

void RequestManager::start()
{
  icov::assert::not_null(m_blockProcessor.get(),
                         "Block processor is not allocated");
  icov::assert::not_null(m_accessBlocks.get(),
                         "Access block manager is not allocated");
  icov::assert::not_null(m_picture.get(), "Picture is not allocated");
  m_accessBlocks->buildAccessBlocks(*m_picture, *m_viewport);
  m_request.viewpoint.angleX = std::numeric_limits<float>::max();
  m_request.viewpoint.angleY = std::numeric_limits<float>::max();
  m_request.frameID = std::numeric_limits<int>::max();
  m_previousDecoded.resize(m_picture->blockCount(), 0);
}

void icov::omnidirectional_layer::RequestManager::processBlock(bool is_access_block,
                                                               const icov::omnidirectional_layer::Block &current_block)
{
  IBlockPredictionManager::prediction_t prediction = 0;

  try {
    prediction =
        (is_access_block
             ? 0
             : m_blockPrediction->getPrediction(current_block, *m_ordering));

    if (m_blockProcessor) {
      m_blockProcessor->processBlock(
          request(), current_block, prediction,
          m_previousDecoded[m_picture->getBlockIndexPos(
              current_block.getIdSelf())] -
              1);
    } else {
      ICOV_TRACE << "Empty processing block " << current_block.getIdSelf()
                 << " with prediction id " << prediction;
    }

    m_previousDecoded[m_picture->getBlockIndexPos(current_block.getIdSelf())] =
        m_currentFrame + 1;

  } catch (...) {
    ICOV_ERROR << "Error while processing block " << current_block.getIdSelf()
               << " with prediction id " << prediction;
    throw;
  }
}

void icov::omnidirectional_layer::RequestManager::processViewport(
    bool usePreviousOrdering) {

  auto current_block = usePreviousOrdering
                           ? m_picture->getById(m_previousOrdering.front())
                           : m_accessBlocks->findAccessBlock(*m_viewport);
  if (usePreviousOrdering)
    m_previousOrdering.pop();

  bool is_access_block = current_block->isAccessBlock();

#ifdef ICOV_ASSERTION_ENABLED
  icov::assert::check(is_access_block, "Access block flag not set");
  icov::assert::check(m_picture->isValidBlock(current_block),
                      "No access block was found in the viewport");
#endif

  while (m_picture->isValidBlock(current_block)) {

      m_previousOrdering.push(current_block->getIdSelf());

      processBlock(is_access_block, *current_block);

      m_previousDecoded[m_picture->getBlockIndexPos(
          current_block->getIdSelf())] = true;

      // if same position as previous, take next in previous ordering
      // else find next block
      current_block =
          usePreviousOrdering
              ? m_picture->getById(m_previousOrdering.front())
              : m_ordering->getNextBlock(*current_block, *m_viewport);

      if (usePreviousOrdering)
        m_previousOrdering.pop();

      is_access_block = false;
  }

  m_previousOrdering.push(m_picture->getInvalidBlockId());
}

void RequestManager::update(const request_t &r) {
  if (r == m_request)
      return;

  bool use_previous_ordering = (r.viewpoint == m_request.viewpoint);

  if (!use_previous_ordering)
      m_previousOrdering = std::queue<Block::id_t>();

  m_currentFrame = r.frameID;

#if ICOV_ASSERTION_ENABLED
  icov::assert::not_null(m_ordering.get(), "Ordering manager cannot be null");
  icov::assert::not_null(m_viewport.get(), "Viewport manager cannot be null");
  icov::assert::not_null(m_accessBlocks.get(),
                         "Access blocks manager manager cannot be null");

  icov::assert::not_null(m_blockPrediction.get(),
                         "Predictions manager manager cannot be null");
#endif

  m_request = r;

  m_ordering->reset();
  m_viewport->viewportGeometry().setPov(
      angle_radian(r.viewpoint.angleX, r.viewpoint.angleY));

  if (m_blockProcessor)
      m_blockProcessor->beforeOrderingLoop(request());

  processViewport(use_previous_ordering);

  if (m_blockProcessor)
      m_blockProcessor->afterOrderingLoop(request());

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
} // namespace omnidirectional_layer
} // namespace icov
