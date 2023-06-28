/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup ICOV
 */

/** @file BlockPrediction.h
 *  @brief BlockPrediction conveying BlockPrediction Units (VPU) for VCL data
 *  @ingroup common
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.0.3
 *  @date 2023
 *  @copyright Copyright (C) 2023 by Inria. All rights reserved.
 * The copyright in this software is being made available under
 * the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 * <br/>
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * <br/>
 * <b> For using ICOV under terms that will not comply with the GNU
 * AGPL, please contact Inria about acquiring an ICOV Professional
 * Edition License. </b>
 * <br/>
 * See contact section in README file for more information.
 * <br/>
 * This software was developed at:
 * Inria Rennes - Bretagne Atlantique
 * Campus Universitaire de Beaulieu
 * 35042 Rennes Cedex
 * France
 * <br/>
 * If you have questions regarding the use of this software, please contact
 * Inria using the contact section in the README file.
 * <br/>
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *****************************************************************************/
#pragma once

#include "Block.h"

#include "icov_exception.h"
#include "omnidirectional_layer/interface/IBlockOrdering.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include "omnidirectional_layer/interface/IPicture.h"
#include <boost/algorithm/string/trim.hpp>

namespace icov {
namespace omnidirectional_layer {
///////////////////////////////////////////////////////////////////////////////
///
/// @class BlockPrediction
/// @brief Manages the different prediction scenarios for a block
///
///////////////////////////////////////////////////////////////////////////////
class BlockPrediction : public IBlockPredictionManager {
public:
  enum ePredType {
    UNKNOWN = -1,
    NONE = 0,
    RIGHT = (1 << (Block::RIGHT - 1)),
    LEFT = (1 << (Block::LEFT - 1)),
    TOP = (1 << (Block::UP - 1)),
    BOTTOM = (1 << (Block::DOWN - 1)),
    TOP_RIGHT = TOP | RIGHT,
    TOP_LEFT = TOP | LEFT,
    BOTTOM_RIGHT = BOTTOM | RIGHT,
    BOTTOM_LEFT = BOTTOM | LEFT,
    INTER = BOTTOM_LEFT + 10,
    MAX,
    ERROR_PRED
  };

  enum eBorderType {
    BORDER_UNKNOWN = -1,
    BORDER_NONE = 0,
    BORDER_TOP,
    BORDER_BOTTOM,
    COUNT
  };

  static const uint8_t SI_Count = 8;
  static const uint8_t PredictionCount = SI_Count + 2;
  static std::string toString(BlockPrediction::ePredType num);
  static ePredType toPredType(uint num);
  static std::vector<BlockPrediction::ePredType>
  getAvailablePredictions(eBorderType b = BORDER_NONE);
  static eBorderType getBorder(const Block &b);
  static bool isValidPrediction(BlockPrediction::ePredType p);

  BlockPrediction() = default;
  ~BlockPrediction() = default;

  // IBlockPrediction interface
public:
  IBlockPredictionManager::prediction_t
  getPrediction(const Block &b, const IBlockOrdering &ordering) const override;

  std::vector<IBlockPredictionManager::prediction_t>
  getAvailablePredictions(const Block &b) const override;
};

inline std::vector<IBlockPredictionManager::prediction_t>
BlockPrediction::getAvailablePredictions(const Block &b) const {

  auto v0 = BlockPrediction::getAvailablePredictions(getBorder(b));
  std::vector<IBlockPredictionManager::prediction_t> v(v0.begin(), v0.end());
  if (b.isAccessBlock())
    v.insert(v.begin(), ePredType::NONE);

  return v;
}

inline std::string BlockPrediction::toString(BlockPrediction::ePredType pred) {
  switch (pred) {
  case BlockPrediction::UNKNOWN:
    return "UNKNOWN";
  case BlockPrediction::NONE:
    return "NONE";
  case BlockPrediction::INTER:
    return "INTER";
  case BlockPrediction::MAX:
    return "MAX";
  case BlockPrediction::ERROR_PRED:
    return "ERROR";
  default: {
    std::stringstream predStr;

    if (pred & BlockPrediction::TOP) {
      predStr << "TOP ";
    }
    if (pred & BlockPrediction::BOTTOM) {
      predStr << "BOTTOM ";
    }
    if (pred & BlockPrediction::LEFT) {
      predStr << "LEFT ";
    }
    if (pred & BlockPrediction::RIGHT) {
      predStr << "RIGHT ";
    }
    auto result = predStr.str();
    boost::trim_right(result);
    return result;
  }
  }
}

inline BlockPrediction::ePredType BlockPrediction::toPredType(uint num) {
  ePredType res = static_cast<ePredType>(num);

  switch (res) {
  case BlockPrediction::UNKNOWN:
  case BlockPrediction::NONE:
  case BlockPrediction::RIGHT:
  case BlockPrediction::LEFT:
  case BlockPrediction::TOP:
  case BlockPrediction::BOTTOM:
  case BlockPrediction::TOP_RIGHT:
  case BlockPrediction::TOP_LEFT:
  case BlockPrediction::BOTTOM_RIGHT:
  case BlockPrediction::BOTTOM_LEFT:
  case BlockPrediction::INTER:
  case BlockPrediction::MAX:
    return res;
  default:
    return ERROR_PRED;
  }
}

inline std::vector<BlockPrediction::ePredType>
BlockPrediction::getAvailablePredictions(eBorderType b) {
  std::vector<BlockPrediction::ePredType> v;

  switch (b) {
  case BORDER_NONE:
    v = {
        BlockPrediction::RIGHT,        BlockPrediction::LEFT,
        BlockPrediction::TOP,          BlockPrediction::BOTTOM,
        BlockPrediction::TOP_RIGHT,    BlockPrediction::TOP_LEFT,
        BlockPrediction::BOTTOM_RIGHT, BlockPrediction::BOTTOM_LEFT,
    };
    break;
  case BlockPrediction::BORDER_TOP:
    v = {
        BlockPrediction::RIGHT,       BlockPrediction::LEFT,
        BlockPrediction::BOTTOM,      BlockPrediction::BOTTOM_RIGHT,
        BlockPrediction::BOTTOM_LEFT,
    };
    break;
  case BlockPrediction::BORDER_BOTTOM:
    v = {
        BlockPrediction::RIGHT,    BlockPrediction::LEFT,
        BlockPrediction::TOP,      BlockPrediction::TOP_RIGHT,
        BlockPrediction::TOP_LEFT,
    };
    break;
  default:
    ICOV_THROW_INVALID_ARGUMENT("Bad border type " + std::to_string(b));
    break;
  }
  return v;
}

inline IBlockPredictionManager::prediction_t
BlockPrediction::getPrediction(const Block &b,
                               const IBlockOrdering &ordering) const {
  ePredType pred = NONE;

  if (ordering.isBlockProcessed(b.getId(Block::UP))) {
    if (ordering.isBlockProcessed(b.getId(Block::LEFT))) {
      pred = TOP_LEFT;
    } else if (ordering.isBlockProcessed(b.getId(Block::RIGHT))) {
      pred = TOP_RIGHT;
    } else {
      pred = TOP;
    }
  } else if (ordering.isBlockProcessed(b.getId(Block::DOWN))) {
    if (ordering.isBlockProcessed(b.getId(Block::LEFT))) {
      pred = BOTTOM_LEFT;
    } else if (ordering.isBlockProcessed(b.getId(Block::RIGHT))) {
      pred = BOTTOM_RIGHT;
    } else {
      pred = BOTTOM;
    }
  } else if (ordering.isBlockProcessed(b.getId(Block::LEFT))) {
    pred = LEFT;
  } else if (ordering.isBlockProcessed(b.getId(Block::RIGHT))) {
    pred = RIGHT;
  }

#ifdef ICOV_ASSERTION_ENABLED
  icov::assert::check(pred != NONE, "Prediction cannot be empty");
#endif

  return static_cast<IBlockPredictionManager::prediction_t>(pred);
}

inline BlockPrediction::eBorderType BlockPrediction::getBorder(const Block &b) {
  eBorderType border = eBorderType::BORDER_NONE;
  if (!b.parent()->blockExists(b.getId(Block::UP))) {
    border = eBorderType::BORDER_TOP;
  } else if (!b.parent()->blockExists(b.getId(Block::DOWN))) {
    border = eBorderType::BORDER_BOTTOM;
  }
  return border;
}

inline bool BlockPrediction::isValidPrediction(ePredType p) {
  if (p == ePredType::ERROR_PRED || p == ePredType::MAX ||
      p == ePredType::UNKNOWN)
    return false;
  return true;
}
} // namespace omnidirectional_layer
} // namespace icov
