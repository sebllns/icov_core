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

#pragma once

#include "common.h"
#include <boost/any.hpp>

/****************************************************************************/
/** @ingroup common
 *  @brief Defines the interface for any IMedia object sent to extractor.
 */

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
protected:
  int m_type;
  std::string m_path;
  float m_width;
  float m_height;
  int m_frameCount;

  IMedia(const std::string &path, int media_type = (int)eMediaType::UNKNOWN)
      : m_type(media_type), m_path(path), m_width(0), m_height(0),
        m_frameCount(0) {}

public:
  inline float getWidth() const { return m_width; }
  inline float getHeight() const { return m_height; }
  inline int getType() const { return m_type; }
  inline int getFrameCount() const { return m_frameCount; }
  inline const std::string getPath() const { return m_path; }
  inline virtual const boost::any getProperty(std::string id) {
    return boost::any(id);
  }

  virtual void setFrame(int frame) = 0;
  virtual ~IMedia() = default;
};

class IImage : public IMedia {
public:
  IImage(const std::string &path) : IMedia(path, (int)eMediaType::IMAGE) {}
  virtual ~IImage() = default;
};

class IVideo : public IMedia {
public:
  IVideo(const std::string &path) : IMedia(path, (int)eMediaType::VIDEO) {}
  virtual ~IVideo() = default;
};

class IModel3D : public IMedia {
public:
  IModel3D(const std::string &path) : IMedia(path, (int)eMediaType::MODEL_3D) {}
  virtual ~IModel3D() = default;
};

class IIcov : public IMedia {
public:
  IIcov(const std::string &path) : IMedia(path, (int)eMediaType::ICOV) {}
  virtual ~IIcov() = default;
};

} // namespace icov
