/****************************************************************************/
/** @addtogroup icov-core
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup icov-core
 */

/** @file helpers.cpp
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
#include "helpers.h"
#include "Tools/Code/LDPC/AList/AList.hpp"

#include <boost/filesystem.hpp>
#include <boost/exception/diagnostic_information.hpp>

namespace fs = boost::filesystem;

namespace icov::helpers
{
int maths::pow2(int k)
{
    return 1 << k;
}

std::string path::get_data_folder(const std::string &root, int code_size, bool auto_create)
{
    fs::path path(root);
    path /= std::to_string(code_size);

    path = fs::absolute(path).make_preferred();
    if(auto_create)
    {
        try_create_directories(path.string());
    }
    return path.string();
}

bool path::try_create_directories(const std::string &path)
{
    bool created = false;
    try
    {
        created = fs::create_directories(path);
    }
    catch (...)
    {
        created = false;
        std::cerr << "Error: could not create directory " << path << std::endl;
        std::cerr << boost::current_exception_diagnostic_information() << std::endl;
    }
    return created;
}

std::string path::combine(const std::string &path1, const std::string &path2)
{
    return  fs::absolute(fs::path(path1).append(path2).make_preferred()).string();
}

void io::writeAlist(const std::string &filename, const AffMatrix &data)
{
    std::ofstream alist_file(filename);
    if (!alist_file.is_open())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("cannot open file " + filename));
    }

    aff3ct::tools::AList::write(data, alist_file);
}

void path::assert_directory_exists(const std::string& path)
{
    if(!fs::is_directory(fs::absolute(path).make_preferred()))
    {
        BOOST_THROW_EXCEPTION(std::invalid_argument("Is not a valid directory: " + path));
    }
}

uint32_t maths::binary_length(int val)
{
    return  std::floor(std::log2f(std::abs(val))) + 1;
}

int rand::range(int min_included, int max_included)
{
    return  min_included + (std::rand() % static_cast<int>(max_included - min_included + 1));
}

void rand::init()
{
    srand((unsigned) time(0));
}

float rand::normalized()
{
    return static_cast <float> (std::rand()) / static_cast <float> (RAND_MAX);
}

float maths::normalize(float val, float min, float max)
{
    return (val-min) / (max-min);
}

float maths::wrap(float val, float min, float max)
{
    while (val < min)
    {
        val += (max-min);
    }

    while (val > max)
    {
        val -= (max-min);
    }

    return val;

    //return val - (min - max) * floor(val / (max - min));
}

void assert::check(bool expr)
{
    if(!expr) BOOST_THROW_EXCEPTION(std::invalid_argument("Assertion is invalid: checked expression is not true"));
}

}
