#include "IcovAccessBlockManager.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/interface/IPicture.h"
#include "omnidirectional_layer/interface/IViewportManager.h"

namespace icov {
namespace omnidirectional_layer {
void IcovAccessBlockManager::buildAccessBlocks(IPicture &pic,
                                               IViewportManager &vpm) {
  m_invalidBlock = pic.invalidBlock();

  auto pov = vpm.viewportGeometry().getPov();

  for (auto it = std::begin(pic.icus()); it != std::end(pic.icus()); ++it) {
    vpm.viewportGeometry().setPov(angle_radian(
        pic.angleX(it->center_u()), pic.angleY(it->center_v()) - M_PI_2));

    if (!pic.isValidBlock(findAccessBlock(vpm))) {
      m_blocks.push_back(it);
      pic.setAccessBlockFlag(it->getIdSelf());
    }
  }

  vpm.viewportGeometry().setPov(pov);
}
} // namespace omnidirectional_layer
} // namespace icov
