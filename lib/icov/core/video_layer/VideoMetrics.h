/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup ICOV
 */

/** @file metrics.h
 *  @brief Metrics for ICOV coder
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

#include "icov_request.h"
#include "icov_types.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include "omnidirectional_layer/interface/IPicture.h"
#include <boost/serialization/singleton.hpp>
#include <limits>
#include <numeric>
#include <vector>

namespace icov
{
namespace video_layer
{
class VideoMetrics : public boost::serialization::singleton<VideoMetrics>
{
    friend class boost::serialization::singleton<VideoMetrics>;

  public:
    struct block_infos_t
    {
        frame_t frame;
        omnidirectional_layer::Block::id_t id;
        omnidirectional_layer::IBlockPredictionManager::prediction_t prediction;
        BitLength encodedBitlength;
        BitLength decodedBitlength;
        float mse;

        void reset()
        {
            frame = 0;
            id = 0;
            prediction = 0;
            encodedBitlength = 0;
            decodedBitlength = 0;
            mse = 0.0f;
        }

        Rate rate() const
        {
            if (decodedBitlength)
            {
                encodedBitlength == 0 ? std::numeric_limits<Rate>::infinity() : 0;
            }
            return static_cast<Rate>(encodedBitlength) / static_cast<Rate>(decodedBitlength);
        }
    };

    request_t request;
    std::vector<block_infos_t> metrics;

    block_infos_t &getBlock(const omnidirectional_layer::Block &b)
    {
        return metrics[b.parent()->getBlockIndexPos(b.getIdSelf())];
    }

    const block_infos_t &getBlock(const omnidirectional_layer::Block &b) const
    {
        return metrics[b.parent()->getBlockIndexPos(b.getIdSelf())];
    }

    size_t blockCount() const
    {
        return metrics.size();
    }

    BitLength viewportEncodedBitlength() const
    {
        return std::accumulate(metrics.cbegin(), metrics.cend(), 0,
                               [](int sum, const block_infos_t &b) { return sum + b.encodedBitlength; });
    }

    BitLength viewportDecodedBitlength() const
    {
        return std::accumulate(metrics.cbegin(), metrics.cend(), 0,
                               [](int sum, const block_infos_t &b) { return sum + b.decodedBitlength; });
    }

    Rate viewportRate() const
    {
        return static_cast<Rate>(viewportEncodedBitlength()) / static_cast<Rate>(viewportDecodedBitlength());
    }
};
} // namespace video_layer
} // namespace icov
