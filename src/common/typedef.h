/****************************************************************************/
/** @addtogroup icov-core 
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup icov-core
 */

/** @file typeDef.h
 *  @brief Defines macros, basic types, new types and enumerations.
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

#ifndef TYPEDEF_H
#define TYPEDEF_H

#ifndef COMMON_H
#error Include common.h not typedef.h
#endif

#include <vector>
#include <utility>
#include <sstream>
#include <cstddef>
#include <cstring>
#include <assert.h>
#include <cassert>

namespace icov
{
  typedef uint8_t byte;
  typedef uint8_t int8b;

  //typedef std::basic_streambuf<icov::byte> bitstreambuf;

  /// Base class for @c icov::byte output streams.
  typedef std::basic_ostream<icov::byte> obitstream;

  class bitstreambuf : public std::basic_streambuf<icov::byte>
  {

  public:
    typedef std::vector<char_type>::size_type size_type;
    using Base = std::basic_streambuf<icov::byte>;
    bitstreambuf() : m_buffer(), read_pos(0)
    {
      //Base::setp(&(*m_buffer.begin()), &(*(m_buffer.end() - 1))); // set std::basic_streambuf
      // put area pointers to work with 'buffer_'
    }

    void seekg(size_type pos)
    {
      read_pos = pos;
    }

    int_type overflow(int_type ch)
    {
      if (ch != EOF)
        m_buffer.push_back(ch);
      return traits_type::to_int_type(ch);
    }

    std::streamsize xsputn(const char_type *s, std::streamsize num)
    {
      m_buffer.insert(m_buffer.end(), s, s + num);
      return num;
    }

    std::streamsize xsgetn(
        char_type *s,
        std::streamsize n)
    {
      if (read_pos < m_buffer.size())
      {
        const size_type num = std::min<size_type>(n, m_buffer.size() - read_pos);
        std::memcpy(s, &m_buffer[read_pos], num);
        read_pos += num;
        return num;
      }
      return 0;
    }

    int size()
    {
      return m_buffer.size();
    }

  private:
    std::vector<char_type> m_buffer;
    size_type read_pos;
  };

  // ====================================================================================================================
  // Enumeration
  // ====================================================================================================================

  enum LogType
  {
    LOG_TYPE_VERBOSE = 0,
    LOG_TYPE_DEBUG = 1,
    LOG_TYPE_INFO,
    LOG_TYPE_WARNING,
    LOG_TYPE_ERROR,
    LOG_TYPE_EXCEPTION,
    LOG_TYPE_MAX
  };

  enum ContentType
  {
    CONTENT_TYPE_VIDEO = 0,
    CONTENT_TYPE_MESH = 1,
    NUM_CONTENT_TYPE,
  };

  /// chroma formats (according to how the monochrome or the color planes are intended to be coded)
  enum ChromaFormat
  {
    CHROMA_400 = 0,
    CHROMA_420 = 1,
    CHROMA_422 = 2,
    CHROMA_444 = 3,
    NUM_CHROMA_FORMAT = 4
  };

  enum ChannelType
  {
    CHANNEL_TYPE_LUMA = 0,
    CHANNEL_TYPE_CHROMA = 1,
    MAX_NUM_CHANNEL_TYPE = 2
  };

#define CH_L CHANNEL_TYPE_LUMA
#define CH_C CHANNEL_TYPE_CHROMA

  enum ComponentID
  {
    COMPONENT_Y = 0,
    COMPONENT_Cb = 1,
    COMPONENT_Cr = 2,
    MAX_NUM_COMPONENT = 3,
    JOINT_CbCr = MAX_NUM_COMPONENT,
    MAX_NUM_TBLOCKS = MAX_NUM_COMPONENT
  };

#define MAP_CHROMA(c) (ComponentID(c))

  enum ModeType
  {
    MODE_TYPE_ALL = 0,
    MODE_TYPE_INTER = 1,
    MODE_TYPE_INTRA = 2,
  };

  /// supported prediction type
  enum PredMode
  {
    MODE_INTER = 0, ///< inter-prediction mode
    MODE_INTRA = 1, ///< intra-prediction mode
    NUMBER_OF_PREDICTION_MODES = 3,
  };

  /// motion vector predictor direction used in AMVP
  enum MvpDir
  {
    MD_LEFT = 0,    ///< MVP of left block
    MD_ABOVE,       ///< MVP of above block
    MD_ABOVE_RIGHT, ///< MVP of above right block
    MD_BELOW_LEFT,  ///< MVP of below left block
    MD_ABOVE_LEFT   ///< MVP of above left block
  };

  enum TransformDirection
  {
    TRANSFORM_FORWARD = 0,
    TRANSFORM_INVERSE = 1,
    TRANSFORM_NUMBER_OF_DIRECTIONS = 2
  };

  enum NalUnitType
  {
    NAL_UNIT_CODED_VPU = 0, // 0
    NAL_UNIT_SOV,           // 1
    NAL_UNIT_CPS,           // 2
    NAL_UNIT_EOC,           // 3

    NAL_UNIT_UNSPECIFIED_001,
    NAL_UNIT_UNSPECIFIED_002,
    NAL_UNIT_UNSPECIFIED_003,
    NAL_UNIT_UNSPECIFIED_004,
    NAL_UNIT_INVALID
  };

#endif
}