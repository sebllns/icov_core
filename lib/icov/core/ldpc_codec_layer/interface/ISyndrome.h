/****************************************************************************/
/** @addtogroup icov-core
 * ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup interactive-coder
 *  @ingroup common
 */

/** @file ISyndrome.h
 *  @brief Defines the syndrome manager class
 *  @ingroup interactive-coder
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 0.2.1
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

#ifndef ISYNDROME_H
#define ISYNDROME_H

#include "common.h"
#include "ldpc_codec_layer/LLRProvider.h"

namespace icov {

class ISyndromeManager {
public:
  ISyndromeManager(int _n) : n(_n){};
  virtual ~ISyndromeManager() = default;

  //! Compute syndrome accumulation before encoding.
  /*!
    \param source_x Source to be encoded.
  */
  virtual void init_accumulation(const BitVec &source_x) = 0;

  //! Compute an incremental syndrome for given code number.
  /*!
    \param o_stored_syndrome Output stored syndrome to fill (must be a size of
    n). \param numcode The code number for which the syndrome will be computed.
    \param llr_provider Provides the llr and src values (needed for doping).
  */
  virtual void compute_stored(Syndrome &o_stored_syndrome, int numcode,
                              LLRProvider &llr_provider, const BitVec &src) = 0;

  //! Compute the syndrome needed for decoding from a given incremental
  //! syndrome.
  /*!
    \param o_syndrome Syndrome to fill, needed for decoding (must be a size of
    n). \param stored_syndrome Incremental syndrome to be processed. \param
    numcode The code number for which the syndrome will be computed. \param
    llr_provider Provides the llr values (needed for doping).
  */
  virtual void compute_deacc(Syndrome &o_syndrome,
                             const Syndrome &stored_syndrome, int numcode,
                             LLRProvider &llr_provider) = 0;

  virtual std::string get_type() const = 0;

  virtual int get_m(int numcode) const { return numcode; }

  virtual int get_n() const { return this->n; }

  virtual void reset_doping() {}

protected:
  uint32_t n;
};
} // namespace icov

#endif // ISYNDROME_H
