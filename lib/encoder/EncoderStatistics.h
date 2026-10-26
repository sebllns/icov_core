/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup Encoder
 *  ICOV Encoder Lib
 *  @ingroup ICOV
 */

/** @file EncoderStatistics.h
 *  @brief Defines an actual ICOV EncoderStatistics.
 *  @ingroup EncoderStatistics
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.5.0
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

#pragma once

#include <boost/property_tree/ptree.hpp>

#include <cstdint>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace icov {
namespace encoder {
class EncoderStatistics {
public:
  EncoderStatistics();
  ~EncoderStatistics() = default;

  void write(std::ostream &output) const;
  boost::property_tree::ptree toTree() const;

  unsigned long getTotal_frames() const;
  void setTotal_frames(unsigned long newTotal_frames);

  unsigned long getTotal_block() const;
  void setTotal_block(unsigned long newTotal_block);
  unsigned long getTotal_access_block() const;
  void setTotal_access_block(unsigned long newTotal_access_block);
  unsigned long long getTotal_bits_syndrome() const;
  void setTotal_bits_syndrome(unsigned long long newTotal_bits_syndrome);
  const std::map<float, unsigned long> &getAll_intra_rates() const;
  const std::map<float, unsigned long> &getPredictions_rates() const;
  unsigned long getTemporal_not_used() const;
  void setTemporal_not_used(unsigned long newTemporal_not_used);
  unsigned long getTemporal_enabled() const;
  void setTemporal_enabled(unsigned long newTemporal_enabled);

  void addFrame() { ++total_frames; }
  void addBlock() { ++total_block; }
  void addAccessBlock() { ++total_access_block; }
  void addEncodedRate(float rate) { encoded_rates[rate]++; }
  void addSIRate(float rate) { si_rates[rate]++; }
  void addTemporalPrediction(bool used) {
    if (used)
      ++temporal_enabled;
    else
      ++temporal_not_used;
  }

protected:
  unsigned long total_frames;
  unsigned long total_block;
  unsigned long total_access_block;
  unsigned long long total_bits_syndrome;

  std::map<float, unsigned long, std::less<float>> encoded_rates;
  std::map<float, unsigned long, std::less<float>> si_rates;
  unsigned long temporal_not_used;
  unsigned long temporal_enabled;

  // IEncoder interface
  // protected:
  //   void endEncoding() override;
};
} // namespace encoder
} // namespace icov
