/****************************************************************************/
/** @addtogroup icov-core
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup icov-core
 */

/** @file MatlabTxtMatrix.h
 *  @brief BRIEF_DESCRIPTION
 *  DESCRIPTION
 *  @ingroup common
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.0.1
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

#include "common.h"

class MatlabTxtMatrix
{
public:
    MatlabTxtMatrix();
    template<typename T> static void read_matrix(const std::string &filename, icov::FastMatrix<T> &mat)
    {
        std::ifstream file(filename);

        if (!file.is_open())
        {
            std::stringstream message;
            message << "'filename' couldn't be opened ('filename' = " <<  filename << ").";
            std::cerr<<message.str()<<std::endl;
            throw  std::runtime_error(message.str());
        }

        uint32_t rows = 0;
        uint32_t cols = 0;

        file >> rows;
        file >> cols;

        mat = icov::FastMatrix<T>(rows, cols);
        T val;

        for(int c = 0 ; c < cols ; c++ )
        {
            for(int r = 0 ; r < rows ; r++ )
            {
                file >> val;
                mat.at(r, c) = val;
            }
        }
    }
};

