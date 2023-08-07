/****************************************************************************/
/** @addtogroup CLI
 *  Command Line Interface
 *  @ingroup ICOV
 */

/** @file InteractiveModeBase.h
 *  @brief BRIEF_DESCRIPTION
 *  DESCRIPTION
 *  @ingroup CLI
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

#include "gl_viewer/single360degreecamerahandler.h"
#include "omnidirectional_layer/icov_geometry.h"
#include "omnidirectional_layer/interface/IBlockProcessor.h"
#include "opencv2/core/mat.hpp"
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/map.hpp>
#include <fstream>

namespace fs = boost::filesystem;
namespace po = boost::program_options;

class InteractiveModeBase
{
  public:
    InteractiveModeBase()
    {
    }
    ~InteractiveModeBase() = default;

    void run();

    float getPov_x() const;
    float getPov_y() const;

  private:
    struct Config
    {
        int quit = 27;
        int left = 'q';
        int right = 'd';
        int up = 'z';
        int down = 's';
        int reset = 'r';
        int f1 = -66;
        int f2 = -65;
        int f3 = -64;
        int f4 = -63;
        int f5 = -62;
        int f6 = -61;
        int f7 = -60;
        int f8 = -59;

        float start_pitch = 0.;
        float start_yaw = 0.;

        float offset_pitch = 10.;
        float offset_yaw = 12.;
    };

    Config m_config;
    std::unique_ptr<Single360degreeCameraHandler> viewer;
    std::string fileAddr;
    float fovX_degree;
    int block_size;
    float pov_x;
    float pov_y;

    void move(float offset_pitch, float offset_yaw);
    void moveLeft();
    void moveRight();
    void moveUp();
    void moveDown();
    void reset();

  protected:
    cv::Mat m_outputImage;
    const char *window_name = "icov";
    virtual bool processKey(char key);
};
