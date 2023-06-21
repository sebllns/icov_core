/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup ICOV
 */

/** @file ContentParameterSet.h
 *  @brief ContentParameterSet VCL data for ICOV coder
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

#pragma once

#include "FastArray.h"
#include "common.h"

#include <boost/range/combine.hpp>

#include <algorithm>
#include <limits>
#include <string>
#include <sys/types.h>

#define Z_STEP 16
#define Z_LIMIT 1024

namespace icov {
namespace ldpc_codec_layer {

class QaryDistribution {

public:
  enum QaryParamsIndex {
    QaryParamsIndex_Q = 0,
    QaryParamsIndex_Z_MIN = 1,
    QaryParamsIndex_Z_MAX = 2,
    QaryParamsIndex_COUNT
  };

  /**
   * @brief The params struct
   *
   * A structure for holding parameters for Pz distribution.
   */
  struct quantization_params {
    bool enable_z_quantization;
    std::vector<float> q_values;
    //    quantization_params()
    //        : enable_z_quantization(true),
    //          q_values({0.125, 0.375, 0.5625, 0.6875, 0.78125, 0.84375,
    //          0.90625,
    //                    0.96875}) {}
  };

  QaryDistribution()
      : m_codingflag(false), m_bitplane_count(0), m_zmin(0), m_zmax(0),
        m_q0(1), m_quantization_params{true,
                                       {0.125, 0.375, 0.5625, 0.6875, 0.78125,
                                        0.84375, 0.90625, 0.96875}} {}

  float getRawParam(QaryParamsIndex idx) const {
    switch (idx) {
    case QaryParamsIndex_Q:
      return getRawQ0();
    case QaryParamsIndex_Z_MIN:
      return getRawZmin();
    case QaryParamsIndex_Z_MAX:
      return getRawZmax();
    default:
      return 0.0;
    }
  }

  /**
   * @brief getZmin
   *
   * Gets the minimum value of z.
   *
   * @return an int representing the minimum value of z.
   */
  int getRawZmin() const { return m_zmin; }

  /**
   * @brief getZmax
   *
   * Gets the maximum value of z.
   *
   * @return an int representing the maximum value of z.
   */
  int getRawZmax() const { return m_zmax; }

  /**
   * @brief getQ0
   *
   * Gets the value of q0.
   *
   * @return a float representing the value of q0.
   */
  float getRawQ0() const { return m_q0; }

  /**
   * @brief getZmin
   *
   * Gets the minimum value of z.
   *
   * @return an int representing the minimum value of z.
   */
  int getZmin() const {
    if (enable_z_quantization())
      return m_zmin * Z_STEP - Z_LIMIT;
    return m_zmin;
  }

  /**
   * @brief getZmax
   *
   * Gets the maximum value of z.
   *
   * @return an int representing the maximum value of z.
   */
  int getZmax() const {
    if (enable_z_quantization())
      return m_zmax * Z_STEP - Z_LIMIT;
    return m_zmax;
  }

  /**
   * @brief getZ0
   *
   * Gets the z value associated with q0.
   *
   * @return an int representing the z value associated with q0.
   */
  int getZ0() const {
    return (getZmin() > 0) ? getZmin() : (getZmax() < 0 ? getZmax() : 0);
  }

  /**
   * @brief getQ0
   *
   * Gets the value of q0.
   *
   * @return a float representing the value of q0.
   */
  float getQ0() const {
    if (getZAmplitude() == 0)
      return 1.0; // std::numeric_limits<float>::epsilon()
    if (enable_q_quantization())
      return q_values().at(m_q0);
    return m_q0;
  }

  int getZAmplitude() const { return getZmax() - getZmin(); }

  float getQ1() const {
    return (getZAmplitude() == 0) ? 0.0
                                  : ((1.0 - getQ0()) / (float)getZAmplitude());
  }

  float getPZ(int z) const {
    return (z == getZ0()) ? getQ0()
                          : ((z < getZmin() || z > getZmax()) ? 0.0 : getQ1());
  }

  short getBitplaneCount() const { return m_bitplane_count; }

  void setQ0(float newQ0) {
    if (enable_q_quantization()) {
      m_q0 = icov::maths::quantize(newQ0, q_values());
      if (icov::debug::debug_trace_enabled)
        ICOV_TRACE << "After quantization q=" << m_q0;
    } else {
      m_q0 = newQ0;
    }
  }

  void setZmin(short newZmin) {
    if (enable_z_quantization()) {
      int z_step = Z_STEP;
      int z_limit = Z_LIMIT;
      m_zmin = icov::maths::quantize(newZmin, z_step, -z_limit + 1, z_limit);

      m_zmin = ((m_zmin + Z_LIMIT) / Z_STEP);

      if (icov::debug::debug_trace_enabled)
        ICOV_TRACE << "After quantization zmin=" << m_zmin;
    } else {
      m_zmin = newZmin;
    }
  }

  void setZmax(short newZmax) {
    if (enable_z_quantization()) {
      int z_step = Z_STEP;
      int z_limit = Z_LIMIT;
      m_zmax = icov::maths::quantize(newZmax, z_step, -z_limit, z_limit - 1);

      m_zmax = ((m_zmax + Z_LIMIT) / Z_STEP);

      if (icov::debug::debug_trace_enabled)
        ICOV_TRACE << "After quantization zmax=" << m_zmax;
    } else {
      m_zmax = newZmax;
    }
  }

  void setBitplane_count(short newBitplane_count) {
    m_bitplane_count = newBitplane_count;
  }
  bool enable_z_quantization() const {
    return m_quantization_params.enable_z_quantization;
  }
  void setEnable_z_quantization(bool newEnable_z_quantization) {
    m_quantization_params.enable_z_quantization = newEnable_z_quantization;
  }
  const std::vector<float> &q_values() const {
    return m_quantization_params.q_values;
  }
  void setQ_values(const std::vector<float> &newQ_values) {
    m_quantization_params.q_values = newQ_values;
  }

  bool enable_q_quantization() const {
    return !m_quantization_params.q_values.empty();
  }

  void setExtractedParams(ushort bitplane_count, ushort q0, ushort zmin,
                          ushort zmax) {
    m_bitplane_count = bitplane_count;
    m_zmin = zmin;
    m_zmax = zmax;
    m_q0 = q0;
  }

  ///
  /// \brief set_si for encode mode: set SI and update Pz distribution
  /// parameters \param si \param enable_z_quantization \param q_values
  ///
  void compute_pz_params(const SymboleVec &src_symbols, int bitplane_count,
                         const SIVec &side_info) {

    m_codingflag = false;

    ICOV_ASSERT_MSG(src_symbols.size() == side_info.size(),
                    "Src and SI lengths are not equal");

    size_t n = src_symbols.size();

    // init intrinsic SI parameters
    float q = 0.0;
    int z_min = std::numeric_limits<int>::max();
    int z_max = std::numeric_limits<int>::min();

    for (size_t i = 0; i < n; i++) {
      int z = src_symbols[i] - side_info[i];

      // q = N0/N
      if (z == 0) {
        q += 1.0 / n;
      } else {
        m_codingflag = true;
      }

      // z_min = min(Z[k], 0<=k<N)
      if (z < z_min)
        z_min = z;

      // z_max = max(Z[k], 0<=k<N)
      if (z > z_max)
        z_max = z;
    }

    // check zmax = xmax - ymin
    // check zmin = xmin - ymax

    if (icov::debug::debug_trace_enabled) {
      ICOV_TRACE << "bp=" << bitplane_count << " q=" << q << " zmin=" << z_min
                 << " z_max=" << z_max;
    }

    setBitplane_count(bitplane_count);
    setQ0(q);
    setZmin(z_min);
    setZmax(z_max);
  }

  //  // q
  //  float q0() const;

  //  // q'
  //  float q1() const;

  //  // z associated with q0 = min(|zk|)
  //  int z0() const;

  //..0..0 [q'..q' q q'..q'] 0..0..
  // float pz(int z) const;

  bool pqkb(bool negative, int abs_predec, int l, int b, int yk, float &p0,
            float &p1, float &p) const {
    abs_predec = abs_predec << (l - b);

    p0 = 0.0;
    p1 = 0.0;
    p = 0.0f;

    // IMPLEMENTATION FANGPING
    int imax = (1 << (l - b));
    for (int i = 0; i < imax; i++) {
      int z = (abs_predec + i) * (1 - 2 * negative) - yk;

      p0 += this->getPZ(z);
      p1 += this->getPZ(z + imax * (1 - 2 * negative));
    }

    float pb = p1 + p0;

    if (icov::debug::debug_trace_enabled) {
      if (pb == 0) {

        ICOV_TRACE << "\t\t\t WARNING: pb = (a + b) = 0";
        ICOV_TRACE << "\t\t\t yk=" << yk << " a=" << p0 << " b=" << p1
                   << " imax=" << ((1 << (l - b)));

        ICOV_TRACE << "\t\t\t a = ";
        int imax2 = (1 << (l - b));
        for (int i = 0; i < imax2; i++) {
          int z = (abs_predec + i) * (1 - 2 * negative) - yk;
          ICOV_TRACE << "pz(" << z << ") + ";
        }

        ICOV_TRACE << "\t\t\t b = ";

        for (int i = 0; i < imax2; i++) {
          int z = (abs_predec + i) * (1 - 2 * negative) - yk +
                  imax2 * (1 - 2 * negative);
          ICOV_TRACE << "pz(" << z << ") + ";
        }

      } else {

        if (std::abs(p0 / pb + p1 / pb - 1) > 0.00001) {
          ICOV_TRACE << "\t\t\t WARNING: sum = " << (p0 / pb + p1 / pb)
                     << " != 1";
        }
      }
    }

    p = pb == 0 ? 0.5 : p0 / pb;

    return (pb != 0);
  }

  float pqks(int yk) const {
    int imax = (1 << getBitplaneCount());

    float p0 = 0.0f;
    float p1 = 0.0f;

    // IMPLEMENTATION SEBASTIEN
    for (int i = 0; i < imax; i++) {
      p0 += this->getPZ(i - yk);
      if (i != 0)
        p1 += this->getPZ(-i - yk);
    }

    p1 += p0;
    ICOV_ASSERT_MSG(p1 > 0,
                    "We need p1 > 0 " + std::to_string(getBitplaneCount()));
    return p0 / p1;
  }

  LLRVal llr_qk(float pqk) const {
    LLRVal p = static_cast<LLRVal>(pqk);
    p = std::clamp(p, std::numeric_limits<LLRVal>::epsilon(),
                   (LLRVal)(1.0 - std::numeric_limits<LLRVal>::epsilon()));
    return std::log(p / (1.0 - p));
  }

  void si_to_llr_sign_bit(const SIVec &input, LLRVec &output) const {
    std::transform(
        input.cbegin(), input.cend(), output.begin(),
        [this](icov::SIVal yk) -> icov::LLRVal { return llr_qk(pqks(yk)); });
  }

  void si_to_llr_value_bit(ushort b, const SIVec &input, const BitVec &sign,
                           const SymboleVec &abs_predec, LLRVec &output) const {
    auto r = boost::combine(input, sign, abs_predec);
    std::transform(
        r.begin(), r.end(), output.begin(), [this, b](auto z) -> icov::LLRVal {
          SIVal yk;
          BitVal sk;
          SymboleVal xk;
          boost::tie(yk, sk, xk) = z;
          LLRVal pqk = 0.0, p0 = 0.0, p1 = 0.0;
          pqkb(sk > 0, xk, getBitplaneCount() - 1, b, yk, p0, p1, pqk);
          return llr_qk(pqk);
        });
  }

  const quantization_params &quantizationParams() const {
    return m_quantization_params;
  }
  quantization_params &quantizationParams() { return m_quantization_params; }

  bool codingflag() const { return m_codingflag; }

private:
  bool m_codingflag;
  ushort m_bitplane_count;
  short m_zmin, m_zmax;
  float m_q0;
  quantization_params m_quantization_params;
};

} // namespace ldpc_codec_layer
} // namespace icov
