/****************************************************************************/
/** @addtogroup ICOV 
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */
 
/** @addtogroup common
 *  @ingroup ICOV
 */

/** @file geometry2d.h
 *  @brief Common 2D-geometrical structures
 *  @ingroup common
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

#ifndef GEOMETRY2D_H
#define GEOMETRY2D_H

#include "common.h"

typedef int PosType;
typedef uint32_t SizeType;
struct Position
{
  PosType x;
  PosType y;

  Position()                                   : x(0),  y(0)  { }
  Position(const PosType _x, const PosType _y) : x(_x), y(_y) { }

  bool operator!=(const Position &other)  const { return x != other.x || y != other.y; }
  bool operator==(const Position &other)  const { return x == other.x && y == other.y; }

  Position offset(const Position pos)                 const { return Position(x + pos.x, y + pos.y); }
  Position offset(const PosType _x, const PosType _y) const { return Position(x + _x   , y + _y   ); }
  void     repositionTo(const Position newPos)              { x  = newPos.x; y  = newPos.y; }
  void     relativeTo  (const Position origin)              { x -= origin.x; y -= origin.y; }

  Position operator-( const Position &other )         const { return{ x - other.x, y - other.y }; }
};

struct Size
{
  SizeType width;
  SizeType height;

  Size()                                              : width(0),      height(0)       { }
  Size(const SizeType _width, const SizeType _height) : width(_width), height(_height) { }

  bool operator!=(const Size &other)      const { return (width != other.width) || (height != other.height); }
  bool operator==(const Size &other)      const { return (width == other.width) && (height == other.height); }
  uint32_t area()                             const { return (uint32_t) width * (uint32_t) height; }
  void resizeTo(const Size newSize)             { width = newSize.width; height = newSize.height; }
};

struct Area : public Position, public Size
{
  Area()                                                                         : Position(),       Size()       { }
  Area(const Position &_pos, const Size &_size)                                  : Position(_pos),   Size(_size)  { }
  Area(const PosType _x, const PosType _y, const SizeType _w, const SizeType _h) : Position(_x, _y), Size(_w, _h) { }

        Position& pos()                           { return *this; }
  const Position& pos()                     const { return *this; }
        Size&     size()                          { return *this; }
  const Size&     size()                    const { return *this; }

  const Position& topLeft()                 const { return *this; }
        Position  topRight()                const { return { (PosType) (x + width - 1), y                          }; }
        Position  bottomLeft()              const { return { x                        , (PosType) (y + height - 1) }; }
        Position  bottomRight()             const { return { (PosType) (x + width - 1), (PosType) (y + height - 1) }; }
        Position  center()                  const { return { (PosType) (x + width / 2), (PosType) (y + height / 2) }; }

  bool contains(const Position &_pos)       const { return (_pos.x >= x) && (_pos.x < (x + width)) && (_pos.y >= y) && (_pos.y < (y + height)); }
  bool contains(const Area &_area)          const { return contains(_area.pos()) && contains(_area.bottomRight()); }

  bool operator!=(const Area &other)        const { return (Size::operator!=(other)) || (Position::operator!=(other)); }
  bool operator==(const Area &other)        const { return (Size::operator==(other)) && (Position::operator==(other)); }
};

struct UnitScale
{
  UnitScale()                 : posx( 0), posy( 0), area(posx+posy) {}
  UnitScale( int sx, int sy ) : posx(sx), posy(sy), area(posx+posy) {}
  int posx;
  int posy;
  int area;

  template<typename T> T scaleHor ( const T &in ) const { return in >> posx; }
  template<typename T> T scaleVer ( const T &in ) const { return in >> posy; }
  template<typename T> T scaleArea( const T &in ) const { return in >> area; }

  Position scale( const Position &pos  ) const { return { pos.x >> posx, pos.y >> posy }; }
  Size     scale( const Size     &size ) const { return { size.width >> posx, size.height >> posy }; }
  Area     scale( const Area    &_area ) const { return Area( scale( _area.pos() ), scale( _area.size() ) ); }
};


#endif
