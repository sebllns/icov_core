#include "ViewportManagerCenterPadded.h"

namespace icov {
namespace omnidirectional_layer {
ViewportManagerCenterPadded::ViewportManagerCenterPadded(float padding)
    : m_padding(padding) {}

bool ViewportManagerCenterPadded::isBlockVisible(const Block &b) const {
  return b.is_block_visible(viewportGeometry(), m_padding);
}
} // namespace omnidirectional_layer
} // namespace icov
