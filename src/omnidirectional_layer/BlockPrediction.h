/****************************************************************************/
/** @addtogroup icov-core
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup icov-core
 */

/** @file BlockPrediction.h
 *  @brief BlockPrediction conveying BlockPrediction Units (VPU) for VCL data
 *  @ingroup common
 *  @author Sébastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 0.0.1
 *  @date 2021
 *  @copyright Copyright (C) 2021 by Inria. All rights reserved.
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
#include "omnidirectional_layer/interface/IBlockOrdering.h"


class IBlockPrediction
{
public:
    virtual ~IBlockPrediction() = default;
    virtual uint16_t getPrediction(const Block &b, const IBlockOrdering &ordering) const = 0;
};

/** @class BlockPrediction
 *  @brief BlockPrediction
 *
 *  A BlockPrediction
 */
class BlockPrediction : public IBlockPrediction
{
public: 
    enum predType
    {
        NONE = 0,
        RIGHT = (1 << Block::RIGHT),
        LEFT = (1 << Block::LEFT),
        TOP = (1 << Block::UP),
        BOTTOM = (1 << Block::DOWN),
        TOP_RIGHT = (1 << Block::UP) | (1 << Block::RIGHT),
        TOP_LEFT = (1 << Block::UP) | (1 << Block::LEFT),
        BOTTOM_RIGHT = (1 << Block::DOWN) | (1 << Block::RIGHT),
        BOTTOM_LEFT = (1 << Block::DOWN) | (1 << Block::LEFT),
    };

    BlockPrediction() = default;
    ~BlockPrediction() = default;
    uint16_t getPrediction(const Block &b, const IBlockOrdering &ordering) const override;

};

inline uint16_t BlockPrediction::getPrediction(const Block &b, const IBlockOrdering &ordering) const
{
    predType pred = NONE;

    if(ordering.isBlockProcessed(b.getId(Block::UP)))
    {
        if(ordering.isBlockProcessed(b.getId(Block::LEFT)))
        {
            pred = TOP_LEFT;
        }
        else if(ordering.isBlockProcessed(b.getId(Block::RIGHT)))
        {
            pred = TOP_RIGHT;
        }
        else
        {
            pred = TOP;
        }
    }
    else if(ordering.isBlockProcessed(b.getId(Block::DOWN)))
    {
        if(ordering.isBlockProcessed(b.getId(Block::LEFT)))
        {
            pred = BOTTOM_LEFT;
        }
        else if(ordering.isBlockProcessed(b.getId(Block::RIGHT)))
        {
            pred = BOTTOM_RIGHT;
        }
        else
        {
            pred = BOTTOM;
        }
    }
    else if(ordering.isBlockProcessed(b.getId(Block::LEFT)))
    {
        pred = LEFT;
    }
    else if(ordering.isBlockProcessed(b.getId(Block::RIGHT)))
    {
        pred = RIGHT;
    }

    icov::helpers::assert::check(pred != NONE, "Prediction cannot be empty");

    return static_cast<uint16_t>(pred);
}
