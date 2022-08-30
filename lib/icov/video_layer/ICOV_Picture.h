/****************************************************************************/
/** @addtogroup video_layer
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup video_layer
 *  @ingroup icov-core
 */

/** @file ICOV_Picture.h
 *  @brief ICOV_Picture
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
#include "CommonLib/Picture.h"
#include "ICOV_Storage.h"

namespace icov::video {

class Picture {
protected:
  BoolStorage _recoFlagBuf;

  PelStorage _bufs[NUM_PIC_TYPES];
  UInt _margins[NUM_PIC_TYPES];

public:
  Picture();
  Void destroy();
  ~Picture();

  Void create(const PictureType pictureType, const ChromaFormat &chromaFormat,
              const Size &lumaSize, const unsigned maxCUSize,
              const unsigned margin, const unsigned alignment = 0,
              const bool scaleChromaMargin = true);
  Void create(const PictureType pictureType, const UnitArea &unit);

  PelBuf getOrigBuf(const ComponentID compID) {
    return _bufs[PIC_ORIGINAL].getBuf(compID);
  }
  const CPelBuf getOrigBuf(const ComponentID compID) const {
    return _bufs[PIC_ORIGINAL].getBuf(compID);
  }
  PelBuf getOrigBuf(const CompArea &blk) {
    return _bufs[PIC_ORIGINAL].getBuf(blk);
  }
  const CPelBuf getOrigBuf(const CompArea &blk) const {
    return _bufs[PIC_ORIGINAL].getBuf(blk);
  }
  PelUnitBuf getOrigBuf(const UnitArea &unit) {
    return _bufs[PIC_ORIGINAL].getBuf(unit);
  }
  const CPelUnitBuf getOrigBuf(const UnitArea &unit) const {
    return _bufs[PIC_ORIGINAL].getBuf(unit);
  }
  PelStorage &getOrigBuf() { return _bufs[PIC_ORIGINAL]; }
  const PelStorage &getOrigBuf() const { return _bufs[PIC_ORIGINAL]; }

  PelBuf getRecoBuf(const ComponentID compID) {
    return _bufs[PIC_RECONSTRUCTION].getBuf(compID);
  }
  const CPelBuf getRecoBuf(const ComponentID compID) const {
    return _bufs[PIC_RECONSTRUCTION].getBuf(compID);
  }
  PelBuf getRecoBuf(const CompArea &blk) {
    return _bufs[PIC_RECONSTRUCTION].getBuf(blk);
  }
  const CPelBuf getRecoBuf(const CompArea &blk) const {
    return _bufs[PIC_RECONSTRUCTION].getBuf(blk);
  }
  PelUnitBuf getRecoBuf(const UnitArea &unit) {
    return _bufs[PIC_RECONSTRUCTION].getBuf(unit);
  }
  const CPelUnitBuf getRecoBuf(const UnitArea &unit) const {
    return _bufs[PIC_RECONSTRUCTION].getBuf(unit);
  }
  PelStorage &getRecoBuf() { return _bufs[PIC_RECONSTRUCTION]; }
  const PelStorage &getRecoBuf() const { return _bufs[PIC_RECONSTRUCTION]; }

  BoolBuf getRecoFlagBuf(const ComponentID compID) {
    return _recoFlagBuf.getBuf(compID);
  }
  const CBoolBuf getRecoFlagBuf(const ComponentID compID) const {
    return _recoFlagBuf.getBuf(compID);
  }
  BoolBuf getRecoFlagBuf(const CompArea &blk) {
    return _recoFlagBuf.getBuf(blk);
  }
  const CBoolBuf getRecoFlagBuf(const CompArea &blk) const {
    return _recoFlagBuf.getBuf(blk);
  }
  BoolUnitBuf getRecoFlagBuf(const UnitArea &unit) {
    return _recoFlagBuf.getBuf(unit);
  }
  const CBoolUnitBuf getRecoFlagBuf(const UnitArea &unit) const {
    return _recoFlagBuf.getBuf(unit);
  }
  BoolStorage &getRecoFlagBuf() { return _recoFlagBuf; }
  const BoolStorage &getRecoFlagBuf() const { return _recoFlagBuf; }

  PelBuf getPredBuf(const ComponentID compID) {
    return _bufs[PIC_PREDICTION].getBuf(compID);
  }
  const CPelBuf getPredBuf(const ComponentID compID) const {
    return _bufs[PIC_PREDICTION].getBuf(compID);
  }
  PelBuf getPredBuf(const CompArea &blk) {
    return _bufs[PIC_PREDICTION].getBuf(blk);
  }
  const CPelBuf getPredBuf(const CompArea &blk) const {
    return _bufs[PIC_PREDICTION].getBuf(blk);
  }
  PelUnitBuf getPredBuf(const UnitArea &unit) {
    return _bufs[PIC_PREDICTION].getBuf(unit);
  }
  const CPelUnitBuf getPredBuf(const UnitArea &unit) const {
    return _bufs[PIC_PREDICTION].getBuf(unit);
  }
  PelStorage &getPredBuf() { return _bufs[PIC_PREDICTION]; }
  const PelStorage &getPredBuf() const { return _bufs[PIC_PREDICTION]; }

  PelBuf getBuf(const PictureType &pictureType, const ComponentID compID) {
    return _bufs[pictureType].getBuf(compID);
  }
  const CPelBuf getBuf(const PictureType &pictureType,
                       const ComponentID compID) const {
    return _bufs[pictureType].getBuf(compID);
  }
  PelBuf getBuf(const PictureType &pictureType, const CompArea &blk) {
    return _bufs[pictureType].getBuf(blk);
  }
  const CPelBuf getBuf(const PictureType &pictureType,
                       const CompArea &blk) const {
    return _bufs[pictureType].getBuf(blk);
  }
  PelUnitBuf getBuf(const PictureType &pictureType, const UnitArea &unit) {
    return _bufs[pictureType].getBuf(unit);
  }
  const CPelUnitBuf getBuf(const PictureType &pictureType,
                           const UnitArea &unit) const {
    return _bufs[pictureType].getBuf(unit);
  }
  PelUnitBuf getBuf(const PictureType &pictureType) {
    return _bufs[pictureType];
  }
  const CPelUnitBuf getBuf(const PictureType &pictureType) const {
    return _bufs[pictureType];
  }
};

} // namespace icov::video
