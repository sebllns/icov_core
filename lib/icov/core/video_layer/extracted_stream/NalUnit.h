/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup ICOV
 */

/** @file NalUnit.h
 *  @brief Defines NAL structure needed to convey video (VCL) and non-video
 *(NON-VCL) data
 * +---------------+---------------+
 * |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |F|Z| LayerID   |  Type   | TID |
 * +---------------+---------------+
 *  @ingroup common
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.0.1
 *  @date 2022
 *  @copyright Copyright (C) 2022 by Inria. All rights reserved.
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

class NalUnit {
public:
  /** construct an NALunit structure with given header values. */
  NalUnit(icov::enum_t::NalUnitType nalUnitType, int temporalId = 0,
          uint32_t nuhReservedZeroBit = 0, uint32_t forbiddenZeroBit = 0,
          int nuhLayerId = 0)
      : m_nalUnitType(nalUnitType), m_temporalId(temporalId),
        m_nuhLayerId(nuhLayerId), m_forbiddenZeroBit(forbiddenZeroBit),
        m_nuhReservedZeroBit(nuhReservedZeroBit), m_rbsp(nullptr),
        m_rbsp_length(0) {}

  NalUnit()
      : m_nalUnitType(icov::enum_t::NalUnitType::NAL_UNIT_INVALID),
        m_temporalId(0), m_nuhLayerId(0), m_forbiddenZeroBit(0),
        m_nuhReservedZeroBit(0), m_rbsp(nullptr), m_rbsp_length(0) {}

  void serialize(icov::byte *&dst) const;
  void deserialize(icov::byte *src);

  void setRbsp(icov::byte *data, int length);
  icov::enum_t::NalUnitType getType() const;
  int getRbspLength() const;
  const icov::byte *getRbsp() const;
  void readRbsp(icov::byte *dst, int length, int offset = 0) const;
  icov::byte getRbspByte(int position) const;

private:
  /// 3 bits
  icov::int8b m_temporalId;

  /// 5 bits
  icov::enum_t::NalUnitType m_nalUnitType;

  /// 6 bits
  icov::int8b m_nuhLayerId;

  /// 1 bit
  bool m_nuhReservedZeroBit;

  /// 1 bit
  bool m_forbiddenZeroBit;

  icov::byte *m_rbsp;
  int m_rbsp_length;
};
