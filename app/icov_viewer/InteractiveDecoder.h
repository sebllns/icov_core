/****************************************************************************/
/** @addtogroup CLI
 *  Command Line Interface
 *  @ingroup ICOV
 */

/** @file InteractiveDecoder.h
 *  @brief BRIEF_DESCRIPTION
 *  DESCRIPTION
 *  @ingroup CLI
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.5.0
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

#include "InteractiveModeBase.h"

#include "app_common/decoder/DecoderController.h"
#include "icov_io.h"
#include "icov_path.h"
#include "icov_types.h"
#include "omnidirectional_layer/Block.h"
#include "opencv2/core/mat.hpp"
#include "video_layer/VideoMetrics.h"

#include <fstream>
#include <memory>
#include <queue>

class InteractiveDecoder : public InteractiveModeBase
{
  public:
    InteractiveDecoder(const path::path_t &icov_file, const options::args_t &args);

    const icov_unique_ptr(decoder::SinglePassDecoder) & getDecoder() const;

    const decoder::DecoderController &decoderCtrl() const;

  private:
    std::queue<request_t> m_requests;
    decoder::DecoderController m_decoderCtrl;
    Rate m_viewportRate;
    float m_viewportMse;
    bool m_showSource;
    cv::Mat m_decodedErp;
    cv::Mat m_sourceViewport;
    cv::Mat m_groundTruthDiff;
    cv::Mat m_monitor;

    float m_vp_rate_sum;
    float m_vp_psnr_sum;
    float m_erp_rate_sum;
    float m_erp_psnr_sum;
    int m_req_count;

    frame_t m_frame;
    bool is_playing;
    bool is_decoding;

    enum class drawType {
      BLOCK_RATE,
      VIEWPORT_RATE,
      PREDICTION,
      PREDICTION_CAT,
      SOURCE,
      SOURCE_DIFF,
      DECODED,
    };

    void drawBlocks(drawType type);
    void drawBlock(drawType type, const omnidirectional_layer::Block &b,
                   const video_layer::VideoMetrics::block_infos_t &i,
                   cv::Mat &output, const cv::Rect &roi);

    // InteractiveModeBase interface
  protected:
    void updateF1() override;
    void updateF2() override;
    void updateF3() override;
    void updateF4() override;
    void updateF5() override;
    void updateF6() override;
    bool processKey(char key) override;
};
