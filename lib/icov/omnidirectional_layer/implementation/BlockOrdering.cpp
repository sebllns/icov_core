#include "BlockOrdering.h"
#include "helpers.h"
#include "omnidirectional_layer/interface/IPicture.h"
#include "omnidirectional_layer/interface/IViewportManager.h"

BlockOrdering::BlockOrdering(size_t blocks_count)
    : m_path_info(std::vector<block_path_info>(blocks_count)) {
  m_elements.reserve(blocks_count);
}

std::vector<BlockOrdering::block_path_info>::iterator
BlockOrdering::getPathInfoById(size_t id) {
  ICOV_ASSERT_MSG(blockExists(id), "ID value is " << id);
  return m_path_info.begin() + id - 1;
}

std::vector<BlockOrdering::block_path_info>::const_iterator
BlockOrdering::getPathInfoById(size_t id) const {
  ICOV_ASSERT_MSG(blockExists(id), "ID value is " << id);
  return m_path_info.begin() + id - 1;
}

Block::NeighborDirection
BlockOrdering::findAvailableNeighbor(const Block &current,
                                     const IViewportManager &vpm) {
  auto pic = current.parent();

  Block::NeighborDirection result = Block::COUNT;

  for (uint16_t n = Block::RIGHT; n != Block::COUNT; n++) {
    auto nn = static_cast<Block::NeighborDirection>(n);
    auto n_id = current.getId(nn);

#ifdef ICOV_CORE_DEBUG_TRACE_ENABLED
    ICOV_LOG(trace) << "\n    block " << current.getId(Block::SELF)
                        << ", neighbor " << nn;
#endif

    if (pic->blockExists(n_id) && !isBlockProcessed(n_id)) {
      //            auto n_block = pic->getById(n_id);
      auto path_info = getPathInfoById(n_id);

      ICOV_ASSERT_MSG(path_info->visibility_tested,
                      "No visibility info avalaible");

      if (path_info->is_visible) {
        ICOV_ASSERT_MSG(path_info->si_horizontal || path_info->si_vertical,
                        "No side info avalaible");

        if (path_info->si_horizontal && path_info->si_vertical) {
#ifdef ICOV_CORE_DEBUG_TRACE_ENABLED
          ICOV_LOG(trace) << ": AVAILABLE!";
#endif

          return nn;
        }

        if (result == Block::COUNT)
          result = nn;
      } else {
#ifdef ICOV_CORE_DEBUG_TRACE_ENABLED
        ICOV_LOG(trace) << ": NOT visible...";
#endif
      }
    } else {
#ifdef ICOV_CORE_DEBUG_TRACE_ENABLED
      ICOV_LOG(trace) << ": NOT available...";
#endif
    }
  }

  return result;
}

BlockVector::const_iterator
BlockOrdering::getNextBlock(const Block &current, const IViewportManager &vpm) {
#ifdef ICOV_CORE_DEBUG_TRACE_ENABLED
  ICOV_LOG(trace) << "\nblock " << m_elements.size()
                      << " id = " << current.getId(Block::SELF) << std::endl;
#endif

  auto pic = current.parent();
  auto path_it = getPathInfoById(current.getId(Block::SELF));
  path_it->visibility_tested = true;
  path_it->is_visible = true;
  path_it->is_processed = true;

  if (m_elements.empty())
    m_elements.push_back(current.getId(Block::SELF));

  for (uint16_t dir = Block::RIGHT; dir != Block::COUNT; dir++) {
    Block::NeighborDirection nb_dir =
        static_cast<Block::NeighborDirection>(dir);

    auto nb_id = current.getId(nb_dir);

    if (pic->blockExists(nb_id) && !isBlockProcessed(nb_id)) {
      auto nb_path_info = getPathInfoById(nb_id);

      if (!nb_path_info->visibility_tested) {
        m_elements.push_back(nb_id);
        nb_path_info->is_visible = vpm.isBlockVisible(*(pic->getById(nb_id)));
        nb_path_info->visibility_tested = true;
      }

      if (nb_path_info->is_visible) {
        switch (nb_dir) {
        case Block::RIGHT:
        case Block::LEFT:
          nb_path_info->si_horizontal = true;
          break;
        case Block::UP:
        case Block::DOWN:
          nb_path_info->si_vertical = true;
          break;

        default:
          ICOV_THROW_LOGICAL_EXCEPTION("should not reach this line");
        }
      }
    }
  }

  m_cursor.push(current.getId(Block::SELF));

  Block::NeighborDirection available = Block::SELF;

  do {
    available = findAvailableNeighbor(*pic->getById(m_cursor.top()), vpm);
    if (available == Block::COUNT)
      m_cursor.pop();
  } while (available == Block::COUNT && !m_cursor.empty());

  if (available == Block::COUNT || m_cursor.empty())
    return pic->invalidBlock();

  return pic->getById((pic->getById(m_cursor.top()))->getId(available));
}

void BlockOrdering::reset() {
  //    std::cout << "reset " << m_elements.size() << std::endl;
  for (size_t id : m_elements) {
    getPathInfoById(id)->reset();
  }
  m_elements.clear();
  m_cursor = std::stack<size_t>();
}

bool BlockOrdering::isBlockProcessed(size_t id) const {
  if (!blockExists(id))
    return false;
  return getPathInfoById(id)->is_processed;
}

bool BlockOrdering::blockExists(size_t id) const {
  return id > 0 && id <= m_path_info.size();
}
