#include "BlockOrdering.h"

#include "icov_assert.h"
#include "icov_debug.h"
#include "icov_exception.h"
#include "omnidirectional_layer/interface/IPicture.h"
#include "omnidirectional_layer/interface/IViewportManager.h"
#include <string>

namespace icov {
namespace omnidirectional_layer {

BlockOrdering::BlockOrdering(size_t blocks_count)
    : m_path_info(std::vector<block_path_info>(blocks_count)) {
  m_elements.reserve(blocks_count);
}

std::vector<BlockOrdering::block_path_info>::iterator
BlockOrdering::getPathInfoById(size_t id) {
  ICOV_ASSERT_MSG(blockExists(id), "ID value is " + std::to_string(id));
  return m_path_info.begin() + id - 1;
}

std::vector<BlockOrdering::block_path_info>::const_iterator
BlockOrdering::getPathInfoById(size_t id) const {
  ICOV_ASSERT_MSG(blockExists(id), "ID value is " + std::to_string(id));
  return m_path_info.begin() + id - 1;
}

// This function finds an available neighbor for a given block based on certain
// conditions.
Block::NeighborDirection
BlockOrdering::findAvailableNeighbor(const Block &current,
                                     const IViewportManager &vpm) {
  auto pic = current.parent();

  // Initialize the result to an invalid value.
  Block::NeighborDirection result = Block::COUNT;

  // Iterate through all possible neighbor directions of the current block.
  for (uint16_t n = Block::RIGHT; n != Block::COUNT; n++) {
    auto nn = static_cast<Block::NeighborDirection>(n);
    auto n_id = current.getId(nn);

    // Check if the neighboring block exists, is not processed, and is not
    // hidden.
    if (pic->blockExists(n_id)) {
      if (!isBlockProcessed(n_id)) {
        if (!pic->getById(n_id)->hidden()) {

          // Check if visibility information is available for the neighboring
          // block.
          auto path_info = getPathInfoById(n_id);
          ICOV_ASSERT_MSG(path_info->visibility_tested,
                          "No visibility info avalaible");

          if (path_info->is_visible) {

            // Check if both horizontal and vertical side information is
            // available.
            ICOV_ASSERT_MSG(path_info->si_horizontal || path_info->si_vertical,
                            "No side info avalaible");

            if (path_info->si_horizontal && path_info->si_vertical) {
              // This is the best choice for decoding, so return the neighbor
              // direction.
              if (debug::debug_trace_enabled) {
                ICOV_TRACE << "Block " << current.getIdSelf() << ", neighbor "
                           << Block::neighborDirectionStr(nn) << " ("
                           << current.getId(nn) << ")"
                           << ":  AVAILABLE (best choice) to decode";
              }
              return nn;
            }

            // Set the default choice as the first visible node
            if (result == Block::COUNT) {
              if (debug::debug_trace_enabled) {
                ICOV_TRACE << "Block " << current.getIdSelf() << ", neighbor "
                           << Block::neighborDirectionStr(nn) << " ("
                           << current.getId(nn) << ")"
                           << ": AVAILABLE (default choice) to decode";
              }
              result = nn;
            }

          } else if (debug::debug_trace_enabled) {
            // The block is not visible.
            ICOV_TRACE << "Block " << current.getIdSelf() << ", neighbor "
                       << Block::neighborDirectionStr(nn) << " ("
                       << current.getId(nn) << ")"
                       << ": NOT visible...";
          }
        } else if (debug::debug_trace_enabled) {
          // The neighboring block is hidden
          ICOV_TRACE << "Block " << current.getIdSelf() << ", neighbor "
                     << Block::neighborDirectionStr(nn) << " ("
                     << current.getId(nn) << "): Hidden";
        }
      } else if (debug::debug_trace_enabled) {
        // The neighboring block is already processed
        ICOV_TRACE << "Block " << current.getIdSelf() << ", neighbor "
                   << Block::neighborDirectionStr(nn) << " ("
                   << current.getId(nn) << "): Already processed";
      }
    } else if (debug::debug_trace_enabled) {
      // The neighboring block does not exist
      ICOV_TRACE << "Block " << current.getIdSelf() << ", neighbor "
                 << Block::neighborDirectionStr(nn) << " (" << current.getId(nn)
                 << ")";
    }
  }

  if (debug::debug_trace_enabled) {
    ICOV_TRACE << "Block " << current.getIdSelf()
               << ", next block to process is "
               << Block::neighborDirectionStr(result) << " ("
               << current.getId(result) << ")";
  }

  return result;
}

BlockVector::const_iterator
BlockOrdering::getNextBlock(const Block &current, const IViewportManager &vpm) {
#ifdef ICOV_CORE_DEBUG_TRACE_ENABLED
  ICOV_TRACE << "Search available neighbor for block " << current.getIdSelf();
#endif

  auto pic = current.parent();
  auto path_it = getPathInfoById(current.getIdSelf());
  path_it->visibility_tested = true;
  path_it->is_visible = true;
  path_it->is_processed = true;

  if (m_elements.empty())
    m_elements.push_back(current.getIdSelf());

  for (uint16_t dir = Block::RIGHT; dir != Block::COUNT; dir++) {
    Block::NeighborDirection nb_dir =
        static_cast<Block::NeighborDirection>(dir);

    auto nb_id = current.getId(nb_dir);

    if (pic->blockExists(nb_id) && !isBlockProcessed(nb_id)) {
      auto nb_path_info = getPathInfoById(nb_id);

      if (!nb_path_info->visibility_tested) {
        m_elements.push_back(nb_id);
        const auto b = pic->getById(nb_id);
        nb_path_info->is_visible = !b->hidden() && vpm.isBlockVisible(*b);
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

  m_cursor.push(current.getIdSelf());

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
} // namespace omnidirectional_layer
} // namespace icov
