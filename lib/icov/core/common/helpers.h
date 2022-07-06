/****************************************************************************/
/** @addtogroup icov-core
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup icov-core
 */

/** @file helpers.hpp
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
#pragma once

#define _USE_MATH_DEFINES // for C++
#define BOOST_ENABLE_ASSERT_DEBUG_HANDLER
#define BOOST_STACKTRACE_USE_ADDR2LINE

#include <vector>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sstream>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <stdexcept>    // std::logic_error
#include <iostream>     // std::cerr

#include <boost/stacktrace.hpp>
#include <boost/exception/all.hpp>
#include "common.h"


//#include "Tools/Algo/Matrix/Sparse_matrix/Sparse_matrix.hpp"
//
//using AffMatrix = aff3ct::tools::Sparse_matrix;


namespace boost {
inline void assertion_failed_msg(char const* expr, char const* msg, char const* function, char const* /*file*/, long /*line*/) {
    std::cerr << "Expression '" << expr << "' is false in function '" << function << "': " << (msg ? msg : "<...>") << ".\n"
            << "Backtrace:\n" << boost::stacktrace::stacktrace() << '\n';
    std::abort();
}

inline void assertion_failed(char const* expr, char const* function, char const* file, long line) {
    ::boost::assertion_failed_msg(expr, 0 /*nullptr*/, function, file, line);
}
} // namespace boost


namespace icov::helpers
{

template<class ForwardIt, class T>
void iota(ForwardIt first, ForwardIt last, T value, T step)
{
    while(first != last) {
        *first++ = value;
        value += step;
    }
}

template<typename T> bool lazy_compare_vector(const std::vector<T>& v1, const std::vector<T>& v2)
{
    if(v1.size() != v2.size())
    {
        return false;
    }
    
    
    bool equal = true;
    int i = -1;
    
    while(equal && ++i < (int)v1.size())
    {
        equal = (v1[i] == v2[i]);
    }
    
    return equal;
}


template<typename T> FastMatrix<T> transpose(const FastMatrix<T>& m)
{
    FastMatrix<T> t(m.cols(), m.rows());
    for(int i = 0; i < t.rows() ; i++)
    {
        for(int j = 0; j < t.cols() ; j++)
        {
            t.at(i,j) = m.at(j, i);
        }
    }
    return t;
}

namespace exception
{
typedef boost::error_info<struct tag_stacktrace, boost::stacktrace::stacktrace> traced;

template <class E>
void throw_with_trace(const E& e) {
    BOOST_THROW_EXCEPTION(boost::enable_error_info(e)
            << traced(boost::stacktrace::stacktrace()));
}

#if !defined(NDEBUG) || defined(ICOV_CORE_DEBUG_TRACE_ENABLED)

#define ICOV_THROW_EXCEPTION(e) icov::helpers::exception::throw_with_trace(e)
#else
#define ICOV_THROW_EXCEPTION(e) BOOST_THROW_EXCEPTION(e)
#endif

#define ICOV_THROW_LOGICAL_EXCEPTION(msg) ICOV_THROW_EXCEPTION(std::logic_error(msg))
#define ICOV_THROW_RUNTIME_EXCEPTION(msg) ICOV_THROW_EXCEPTION(std::runtime_error(msg))
#define ICOV_THROW_INVALID_ARGUMENT(msg) ICOV_THROW_EXCEPTION(std::invalid_argument(msg))
#define ICOV_THROW_OUT_OF_RANGE(msg) ICOV_THROW_EXCEPTION(std::range_error(msg))
#define ICOV_THROW_NULLPTR(msg) ICOV_THROW_EXCEPTION(std::invalid_argument(msg))

}

namespace assert
{

#ifndef NDEBUG
#define ICOV_ASSERT_MSG(cond, msg) {\
    if(!(cond))\
{\
    std::cerr << "*******************************************************" << std::endl << msg << std::endl;\
}\
    BOOST_ASSERT(cond);\
}
#else
#define ICOV_ASSERT_MSG(...)
#endif


template<typename T> void range(T val, T min, T max)
{
    if(min > max) BOOST_THROW_EXCEPTION(std::invalid_argument("Required min <= max"));
    
    if(val < min || val > max) BOOST_THROW_EXCEPTION(std::out_of_range("VALUE=" + std::to_string(val) + " while the acceptable range is [" + std::to_string(min) + ", " + std::to_string(max) + "]"));
    
}

void not_null(void *p);

void check(bool expr, const std::string& msg);

}

namespace maths
{
int pow2(int k);
uint32_t binary_length(int val);
float normalize(float val, float min, float max);
float wrap(float val, float min, float max);
float repeat(float val, float min, float max);
}

namespace rand
{
void init();
int range(int min_included, int max_included);
float normalized();
}

namespace io
{
template<typename T> void write_vector(const std::string& filename, const std::vector<T>& data, char sep )
{
    std::ofstream file(filename);
    file << data.size();
    for(int i = 0; i < data.size(); i++)
    {
        file  << sep << data[i];
    }
    file.close();
}


template<typename T> void read_vector(const std::string& filename,  std::vector<T>& data)
{
    std::ifstream file(filename);
    
    if(!file.is_open())
    {
        throw std::invalid_argument("cannot open file " + filename);
    }
    
    int n = 0;
    file >> n;
    data = std::vector<T>(n);
    
    T val;
    for(int i = 0; i < data.size(); i++)
    {
        file  >> val;
        data[i] = val;
    }
    file.close();
}

//void writeAlist(const std::string& filename,  const AffMatrix& data);
}

namespace path
{

void assert_directory_exists(const std::string& path);
bool try_create_directories(const std::string& path);
std::string get_data_folder(const std::string& root, int code_size, bool auto_create = false);
std::string combine(const std::string& path1, const std::string& path2);
}

}
