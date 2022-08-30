/****************************************************************************/
/** @addtogroup video_layer
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup video_layer
 *  @ingroup icov-core
 */

/** @file ICOV_Storage.h
 *  @brief ICOV_Storage
 *  @ingroup video_layer
 *  @author Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
 *  @author Sébastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.0.0
 *  @date 2018, 2022
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
#include "CommonLib/CommonDef.h"
#include "CommonLib/Unit.h"
#include "ldpc_codec_layer/BitSymbolProvider.h"

namespace icov::video {

template <typename T> struct Storage : public UnitBuf<T> {
private:
  struct StorageData {
    T *m_origin[MAX_NUM_COMPONENT];
    unsigned int _refCount;
    StorageData() {
      _refCount = 0;
      for (UInt i = 0; i < MAX_NUM_COMPONENT; i++) {
        m_origin[i] = nullptr;
      }
    }

    void deallocate() {
      //            std::cout << "deallocating Storage data" << std::endl;
      for (UInt i = 0; i < MAX_NUM_COMPONENT; i++) {
        if (m_origin[i]) {
          xFree(m_origin[i]);
          //                delete [] m_origin[i];
          //                m_origin[i] = nullptr;
        }
      }
    }
    void allocate(UInt index, UInt size) {
      m_origin[index] = (T *)xMalloc(T, size);
      //            m_origin[index] = new T[size];
    }
  };
  StorageData *m_d;

public:
  Storage() : m_d(nullptr) {}

  Storage(const Storage &other) : m_d(nullptr) { (*this) = other; }

  Storage(Storage &&other) // double ampersand "&&" declares an rvalue
                           // reference in c++11. it is provided for move
                           // semantics in move constructor
  {
    std::cout << "rvalue reference constructor is used in move constructor"
              << std::endl;
    std::cout << "if it didn't call operator=(Storage &&other) then I have "
                 "to write if by myself"
              << std::endl;
    (*this) = other;
  }

  ~Storage() { destroy(); }

  Storage &operator=(const Storage &other) {
    if (this != &other) {
      if (other.m_d && other.m_d->m_origin[0])
        other.m_d->_refCount++;
      //                *const_cast<unsigned int *> (&other.m_d->_refCount) =
      //                other.m_d->_refCount + 1;
      //                __atomic_fetch_add((unsigned*)(&other.m_d->_refCount),
      //                (unsigned)(1), __ATOMIC_ACQ_REL);

      release();
      this->m_d = other.m_d;
      this->bufs = other.bufs;
      this->chromaFormat = other.chromaFormat;
    }
    return *this;
  }

  Storage &
  operator=(Storage &&other) // double ampersand "&&" declares an rvalue
                             // reference in c++11. it is provided for move
                             // semantics in move constructor
  {
    std::cout << "rvalue reference constructor is used in operator="
              << std::endl;

    if (this == &other)
      return *this;

    release();
    this->m_d = other.m_d;
    this->bufs = other.bufs;
    this->chromaFormat = other.chromaFormat;

    other.m_d = nullptr;
    other.chromaFormat = NUM_CHROMA_FORMAT;
    other.bufs.clear();

    return *this;
  }

  void destroy() {
    if (m_d && m_d->m_origin[0] &&
        m_d->_refCount-- ==
            1) // checking pointer m_origin[0] is enough and more reliable
               // because they always allocated together and somtimes depending
               // on ChromaFormat m_origin[1], m_origin[2] may not be allocated
    {
      m_d->deallocate();

      if (this->m_d)
        delete this->m_d;
    }

    this->m_d = nullptr;
    this->chromaFormat = NUM_CHROMA_FORMAT;
    this->bufs.clear(); // TODO: shall I do this like deallocate?
  }

  void release() { destroy(); }

  bool empty() { return !m_d || !m_d->m_origin[0]; }
  void create(const ChromaFormat &_chromaFormat, const Area &_lumaArea,
              const unsigned _maxCUSize = 0, const unsigned _margin = 0,
              const unsigned _alignment = 0,
              const bool _scaleChromaMargin = true) {
    //        CHECK( !this->bufs.empty(), "Trying to re-create an already
    //        initialized buffer" );

    release();

    this->chromaFormat = _chromaFormat;

    const UInt numCh = getNumberValidComponents(_chromaFormat);

    unsigned extHeight = _lumaArea.height;
    unsigned extWidth = _lumaArea.width;

    if (_maxCUSize) {
      extHeight =
          ((_lumaArea.height + _maxCUSize - 1) / _maxCUSize) * _maxCUSize;
      extWidth = ((_lumaArea.width + _maxCUSize - 1) / _maxCUSize) * _maxCUSize;
    }

    m_d = new StorageData();
    for (UInt i = 0; i < numCh; i++) {
      const ComponentID compID = ComponentID(i);
      const unsigned scaleX = ::getComponentScaleX(compID, _chromaFormat);
      const unsigned scaleY = ::getComponentScaleY(compID, _chromaFormat);

      unsigned scaledHeight = extHeight >> scaleY;
      unsigned scaledWidth = extWidth >> scaleX;
      unsigned ymargin = _margin >> (_scaleChromaMargin ? scaleY : 0);
      unsigned xmargin = _margin >> (_scaleChromaMargin ? scaleX : 0);
      unsigned totalWidth = scaledWidth + 2 * xmargin;
      unsigned totalHeight = scaledHeight + 2 * ymargin;

      if (_alignment) {
        // make sure buffer lines are align
        CHECK(_alignment != MEMORY_ALIGN_DEF_SIZE, "Unsupported alignment");
        totalWidth = ((totalWidth + _alignment - 1) / _alignment) * _alignment;
      }
      UInt area = totalWidth * totalHeight;
      CHECK(!area, "Trying to create a buffer with zero area");

      m_d->allocate(i, area);
      T *topLeft = m_d->m_origin[i] + totalWidth * ymargin + xmargin;
      this->bufs.push_back(AreaBuf<T>(topLeft, totalWidth,
                                      _lumaArea.width >> scaleX,
                                      _lumaArea.height >> scaleY));
    }

    addref();
  }

  void create(const UnitArea &_UnitArea) {
    create(_UnitArea.chromaFormat, _UnitArea.blocks[0]);
  }

  //    void createFromBuf( UnitBuf<      T> buf )
  //    {
  //        this->chromaFormat = buf.chromaFormat;

  //        const UInt numCh = ::getNumberValidComponents( this->chromaFormat );

  //        this->bufs.resize(numCh);

  //        for( UInt i = 0; i < numCh; i++ )
  //        {
  //          AreaBuf<T> cBuf = buf.get( ComponentID( i ) );
  //          this->bufs[i] = AreaBuf<T>( cBuf.bufAt( 0, 0 ), cBuf.stride,
  //          cBuf.width, cBuf.height );
  //        }
  //        // No need to set m_origin, becasue it is NOT a deep copy and the
  //        task of deleting the buffer at the end should be handled by buf
  //        itself
  //    }

  AreaBuf<T> &getBuf(const ComponentID CompID) { return this->bufs[CompID]; }

  const AreaBuf<T> &getBuf(const ComponentID CompID) const {
    return this->bufs[CompID];
  }

  AreaBuf<T> getBuf(const CompArea &blk) {
    const AreaBuf<T> &r = this->bufs[blk.compID];

    CHECKD(rsAddr(blk.bottomRight(), r.stride) >=
               ((r.height - 1) * r.stride + r.width),
           "Trying to access a buf outside of bound!");

    return AreaBuf<T>(r.buf + rsAddr(blk, r.stride), r.stride, blk);
  }

  const AreaBuf<const T> getBuf(const CompArea &blk) const {
    const AreaBuf<T> &r = this->bufs[blk.compID];
    return AreaBuf<const T>(r.buf + rsAddr(blk, r.stride), r.stride, blk);
  }

  UnitBuf<T> getBuf(const UnitArea &unit) {
    return (this->chromaFormat == CHROMA_400)
               ? UnitBuf<T>(this->chromaFormat, getBuf(unit.Y()))
               : UnitBuf<T>(this->chromaFormat, getBuf(unit.Y()),
                            getBuf(unit.Cb()), getBuf(unit.Cr()));
  }

  const UnitBuf<const T> getBuf(const UnitArea &unit) const {
    return (this->chromaFormat == CHROMA_400)
               ? UnitBuf<const T>(this->chromaFormat, getBuf(unit.Y()))
               : UnitBuf<const T>(this->chromaFormat, getBuf(unit.Y()),
                                  getBuf(unit.Cb()), getBuf(unit.Cr()));
  }

private:
  void addref() {
    if (m_d)
      m_d->_refCount++;
  }
};

typedef AreaBuf<bool> BoolBuf;
typedef AreaBuf<const bool> CBoolBuf;
typedef UnitBuf<bool> BoolUnitBuf;
typedef UnitBuf<const bool> CBoolUnitBuf;
typedef Storage<bool> BoolStorage;
typedef Storage<TCoeff> TCoeffStorage;
typedef Storage<TCoeff> QTCoeffStorage; // quantized transform coefficients
typedef Storage<TCoeff>
    IQTCoeffStorage; // inverse quantized transform coefficients

} // namespace icov::video
