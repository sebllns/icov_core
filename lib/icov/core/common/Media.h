/****************************************************************************/
/** @addtogroup icov-core
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup icov-core
 */

/** @file IMedia.h
 *  @brief Defines interface for IMedia object.
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
#include "helpers.h"
#include <boost/any.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

namespace icov {

enum eMediaType {
  UNKNOWN = -1,
  IMAGE = 0,
  VIDEO = 1,
  MODEL_3D = 2,
  ICOV = 3,
  COUNT,
  ERROR
};

class IMedia {
public:
  virtual ~IMedia() = default;
  IMedia(const fs::path &path, int media_type = (int)eMediaType::UNKNOWN)
      : m_type(media_type), m_path(fs::complete(path)), m_width(0), m_height(0),
        m_frameCount(0)

  {
    icov::helpers::path::assert_file_exists(path.string());
    ICOV_ASSERT_MSG(m_type != eMediaType::COUNT && m_type != eMediaType::ERROR,
                    "Media type is unsupported");
    ICOV_WARNING_MSG(m_type != eMediaType::UNKNOWN, "Media type is unknown");
  }

  inline size_t getWidth() const { return m_width; }
  inline size_t getHeight() const { return m_height; }
  inline int getType() const { return m_type; }
  inline int getFrameCount() const { return m_frameCount; }
  inline size_t getPixelCount() const { return m_width * m_height; }

protected:
  int m_type;
  fs::path m_path;
  size_t m_width;
  size_t m_height;
  int m_frameCount;

private:
  IMedia() = default;
  //  IMedia(const IMedia &) = default;
};

/****************************************************************************/
/** @ingroup common
 *  @brief Defines the interface for any IInputMedia.
 */
template <typename T> class IInputMedia : public IMedia {
private:
  IInputMedia() = default;

protected:
  T m_frameData;
  int m_frameID;

public:
  // using IMedia::IMedia;

  IInputMedia(const fs::path &path, int media_type = (int)eMediaType::UNKNOWN)
      : IMedia(path, media_type), m_frameID(0) {}

  virtual void setFrame(int frame) { m_frameID = frame; }
  inline int getFrameID() const { return m_frameID; }

  virtual inline const T &getCurrentFrame() const { return m_frameData; }
  inline const fs::path getPath() const { return m_path; }
  inline virtual const boost::any getProperty(std::string id) {
    return boost::any(id);
  }

  virtual void writeInfos(std::ofstream &output, int key,
                          void *param = nullptr) = 0; //@todo : const ?
  virtual ~IInputMedia() = default;
};

/****************************************************************************/
/** @ingroup common
 *  @brief Defines the interface for any IOutputMedia.
 */
class OutputStream : public IMedia {
private:
  OutputStream() = default;

protected:
public:
};

template <typename T> class IImage : public IInputMedia<T> {
public:
  IImage(const fs::path &path) : IInputMedia<T>(path, (int)eMediaType::IMAGE) {
    /*image = 1 frame only*/
    this->m_frameCount = 1;
  }

  // IMedia interface
public:
  void setFrame(int frame = 0) override {
    this->m_frameID = 0; /*image = 1 frame only*/
  }
};

template <typename T> class IVideo : public IInputMedia<T> {
public:
  IVideo(const fs::path &path) : IInputMedia<T>(path, (int)eMediaType::VIDEO) {}
};

template <typename T> class IModel3D : public IInputMedia<T> {
public:
  IModel3D(const fs::path &path)
      : IInputMedia<T>(path, (int)eMediaType::MODEL_3D) {}
  virtual ~IModel3D() = default;
};

template <typename T> class IIcov : public IInputMedia<T> {
public:
  IIcov(const fs::path &path) : IInputMedia<T>(path, (int)eMediaType::ICOV) {}
  virtual ~IIcov() = default;
};

} // namespace icov
