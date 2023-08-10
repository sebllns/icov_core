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
    struct bgrColor
    {
        static cv::Scalar GREEN()
        {
            return cv::Scalar(0, 255, 0);
        }
        static cv::Scalar RED()
        {
            return cv::Scalar(0, 0, 255);
        }
        static cv::Scalar BLUE()
        {
            return cv::Scalar(255, 0, 0);
        }
        static cv::Scalar VULCAN()
        {
            return cv::Scalar(12, 21, 140);
        }
        static cv::Scalar PINK()
        {
            return cv::Scalar(255, 0, 255);
        }
        static cv::Scalar PURPLE()
        {
            return cv::Scalar(255, 51, 51);
        }
        static cv::Scalar CYAN()
        {
            return cv::Scalar(255, 255, 0);
        }
        static cv::Scalar YELLOW()
        {
            return cv::Scalar(0, 255, 255);
        }
        static cv::Scalar FOREST_GREEN()
        {
            return cv::Scalar(0, 102, 51);
        }
        static cv::Scalar ORANGE()
        {
            return cv::Scalar(0, 128, 255);
        }
        static cv::Scalar WHITE()
        {
            return cv::Scalar(255, 255, 255);
        }
        static cv::Scalar BLACK()
        {
            return cv::Scalar(0, 0, 0);
        }
        static cv::Scalar GRAY()
        {
            return cv::Scalar(96, 96, 96);
        }
        static cv::Scalar PASTEL_RED()
        {
            return cv::Scalar(204, 204, 255);
        }
        static cv::Scalar PASTEL_YELLOW()
        {
            return cv::Scalar(204, 255, 255);
        }
        static cv::Scalar PASTEL_GREEN()
        {
            return cv::Scalar(204, 255, 204);
        }
        static cv::Scalar PASTEL_BLUE()
        {
            return cv::Scalar(255, 255, 204);
        }
        static cv::Scalar PASTEL_PURPLE()
        {
            return cv::Scalar(255, 204, 204);
        }
        static cv::Scalar PASTEL_PINK()
        {
            return cv::Scalar(229, 204, 255);
        }
    };

    struct Config
    {
        int quit = 27;
        int left = 'q';
        int right = 'd';
        int up = 'z';
        int down = 's';
        int reset = 'r';
        int f1 = -66;
        int f2 = -66;
        int f3 = -66;
        int f4 = -66;
        int f5 = -66;
        int f6 = -66;
        int f7 = -66;
        int f8 = -66;

        float start_pitch = 0.;
        float start_yaw = 0.;

        float offset_pitch = 10.;
        float offset_yaw = 12.;
    };

    InteractiveModeBase();
    virtual ~InteractiveModeBase();

    void run();

    float getPov_x() const;
    float getPov_y() const;
    int special_key() const;

    void clear();
    void fill(const cv::Scalar &color);
    void draw();

    const Config &config() const;

  private:
    std::unique_ptr<Single360degreeCameraHandler> viewer;
    std::string fileAddr;
    float fovX_degree;
    int block_size;
    float pov_x;
    float pov_y;
    int m_special_key;

    bool is_special_key(char key) const;
    void move(float offset_pitch, float offset_yaw);
    void moveLeft();
    void moveRight();
    void moveUp();
    void moveDown();
    void reset();

  protected:
    Config m_config;
    cv::Mat m_outputImage;
    const char *window_name = "icov";
    virtual bool processKey(char key);
    virtual void onSpecialKey(int previous, int current);

    virtual void onF1();
    virtual void onF2();
    virtual void onF3();
    virtual void onF4();
    virtual void onF5();
    virtual void onF6();
    virtual void onF7();
    virtual void onF8();

    virtual void updateF1();
    virtual void updateF2();
    virtual void updateF3();
    virtual void updateF4();
    virtual void updateF5();
    virtual void updateF6();
    virtual void updateF7();
    virtual void updateF8();
};

inline int InteractiveModeBase::special_key() const
{
    return m_special_key;
}
