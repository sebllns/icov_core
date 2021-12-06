/****************************************************************************/
/** @addtogroup icov-core
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup icov-core
 */

/** @file PcmData.cpp
 *  @brief BRIEF_DESCRIPTION
 *  DESCRIPTION
 *  @ingroup common
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
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
#include "PcmData.h"

#include <fstream>
#include <sstream>

PcmData::PcmData()
{

}

void PcmData::read_matrix(const string &filepath, AffMatrix &h)
{
    std::ifstream file(filepath);

    if (!file.is_open())
    {
        std::stringstream message;
        message << "'filename' couldn't be opened ('filename' = " <<  filepath << ").";
        throw std::runtime_error(message.str());
    }

    int n_rows  = 0;
    int n_cols = 0;

    file >> n_rows;
    file >> n_cols;

    if((int)h.get_n_rows() != n_rows || (int)h.get_n_cols() != n_cols)
    {
        h.self_resize(n_rows, n_cols, aff3ct::tools::Matrix::Origin::TOP_LEFT);
    }

    for(int i = 0; i < n_rows ; i++)
    {
        int n_ones = 0;
        file >> n_ones;

        for(int j = 0 ; j < n_ones ; j++)
        {
            int one_col = 0;
            file >> one_col;
            h.add_connection(i, one_col - 1);
        }
    }

    file.close();
}

void PcmData::read_matrix(const string &filepath, icov::FastMatrix<bool> &h)
{
    std::ifstream file(filepath);

    if (!file.is_open())
    {
        std::stringstream message;
        message << "'filename' couldn't be opened ('filename' = " <<  filepath << ").";
        throw std::runtime_error(message.str());
    }

    int n_rows  = 0;
    int n_cols = 0;

    file >> n_cols;
    file >> n_rows;

    if((int)h.rows() != n_rows || (int)h.cols() != n_cols)
    {
        h = icov::FastMatrix<bool>(n_rows, n_cols);
    }

    for(int i = 0; i < n_cols ; i++)
    {
        int n_ones = 0;
        file >> n_ones;

        for(int j = 0 ; j < n_ones ; j++)
        {
            int one_col = 0;
            file >> one_col;
            h.at(one_col - 1, i) = 1;
        }
    }

    file.close();
}

void PcmData::read_mapping_vector(const string &filepath, icov::FastMatrix<uint16_t> &m)
{
    std::ifstream file(filepath);

    std::cout << "read_mapping_vector " << filepath << std::endl;;


    if (!file.is_open())
    {
        std::stringstream message;
        message << "'filename' couldn't be opened ('filename' = " <<  filepath << ").";
        throw std::runtime_error(message.str());
    }

    int n_rows  = 0;
    int n_cols = 0;

    file >> n_rows;
    file >> n_cols;

    std::cout <<"V "<< n_cols << ", " << n_rows << std::endl;


    m = icov::FastMatrix<uint16_t>(n_rows, n_cols);

    std::cout << "read\n";


    for(int i = 0; i < n_rows ; i++)
    {
        std::cout << "ROW " <<i << std::endl;
        for(int j = 0 ; j < n_cols ; j++)
        {
            uint16_t value = 0;
            file >> value;
            m.at(i, j) = value - 1;
           // std::cout << i << ", " << j << std::endl;
        }
    }

    std::cout << "end\n";
    file.close();
}


