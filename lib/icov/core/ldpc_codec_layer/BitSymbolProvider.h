#ifndef BITSYMBOLPROVIDER_H
#define BITSYMBOLPROVIDER_H

#include "common.h"

namespace icov {
class BitSymbolProvider {
public:
  /**
   * @brief compute_bits
   * Compute bits from symboles
   * @param input
   * @return
   */
  static Bitplanes compute_bits(const SymboleVec &input);

  /**
   * @brief compute_symboles
   * Compute symboles from bits
   * @param input
   * @return
   */
  static SymboleVec compute_symboles(const Bitplanes &input);

  static const BitSymbolProvider
  init_bsp(const SymboleVec &src_symbols, const SIVec &side_info, int lx,
           bool enable_z_quantization,
           const std::vector<float> &q_values = std::vector<float>());

  BitSymbolProvider(int z_min, int z_max, int y_min, int y_max, float q, int l);

  BitSymbolProvider(const SymboleVec &src_symbols);

  void set_si(const SIVec &si, bool enable_z_quantization,
              const std::vector<float> &q_values = std::vector<float>());

  float z_min() const;
  float z_max() const;

  float y_min() const;
  float y_max() const;

  // q
  float q0() const;

  // q'
  float q1() const;

  // z associated with q0 = min(|zk|)
  int z0() const;

  //..0..0 [q'..q' q q'..q'] 0..0..
  float pz(int z) const;

  float pz(int zmin, int zmax) const;

  // L
  int z_binary_length() const;

  int x_min_estimated() const;
  int x_max_estimated() const;

  int x_binary_length() const;

  bool pqkb(bool negative, int abs_predec, int l, int b, int yk, float &p0,
            float &p1, float &p) const;
  float pqks(bool negative, int yk) const;

  const Bitplanes &src_bits() const { return m_src_bits; }
  //  const Bitplanes &si_bits() const { return m_si_bits; }

  const SymboleVec &src_symboles() const { return m_src_symboles; }
  const SIVec &si_symboles() const { return m_si_symboles; }

  //  Bitplanes &src() { return m_src; }
  //  Bitplanes &si() { return m_si; }

private:
  int m_z_min, m_z_max;
  int m_y_min, m_y_max;
  // q
  float m_q0;

  int m_l;

  SymboleVec m_src_symboles;
  SIVec m_si_symboles;
  Bitplanes m_src_bits;
  //  Bitplanes m_si_bits;
};
} // namespace icov
#endif // BITSYMBOLPROVIDER_H
