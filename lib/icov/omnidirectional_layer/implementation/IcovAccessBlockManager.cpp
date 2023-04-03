#include "IcovAccessBlockManager.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/interface/IPicture.h"
#include "omnidirectional_layer/interface/IViewportManager.h"

IcovAccessBlockManager::IcovAccessBlockManager(angle_radian delta)
    : m_angle_delta(delta) {}

void IcovAccessBlockManager::buildAccessBlocks(IPicture &pic,
                                               IViewportManager &vpm) {
  m_invalidBlock = pic.invalidBlock();

  auto pov = vpm.viewportGeometry().getPov();

  for (auto it = std::begin(pic.icus()); it != std::end(pic.icus()); ++it) {
    vpm.viewportGeometry().setPov(angle_radian(
        pic.angleX(it->center_u()), pic.angleY(it->center_v()) - M_PI_2));

    if (!pic.isValidBlock(findAccessBlock(vpm))) {
      m_blocks.push_back(it);
      pic.setAccessBlockFlag(it->getId(Block::SELF));
    }
  }

  //    for(float angle_y = pic.minAngleY() ; angle_y <= pic.maxAngleY() ;
  //    angle_y += m_angle_delta.y())
  //    {
  //        for(float angle_x = pic.minAngleX() ; angle_x < pic.maxAngleX() ;
  //        angle_x += m_angle_delta.x())
  //        {

  //        }
  //    }

  vpm.viewportGeometry().setPov(pov);
}

const std::string IcovAccessBlockManager::getID() {
  return "aa12d096-261c-4ba4-81b1-c9b7c4ff9ffc";
}
