#pragma once

#include "common.h"

namespace icov {
class BitSymbolProvider {
public:
  struct params {
    bool enable_z_quantization;
    std::vector<float> q_values;
  };

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

  static void inject_bitplane(SymboleVec &output, const BitVec &input,
                              int bp_shift);

  ///
  /// \brief BitSymbolProvider
  /// \param n
  ///
  BitSymbolProvider(int n);

  ///
  /// \brief BitSymbolProvider ctor used in encode mode
  /// \param src_symbols
  ///
  BitSymbolProvider(const SymboleVec &src_symbols);

  ///
  /// \brief set_si for encode mode: set SI and update Pz distribution
  /// parameters \param si \param enable_z_quantization \param q_values
  ///
  void set_si(const SIVec &si, bool enable_z_quantization,
              const std::vector<float> &q_values = std::vector<float>());

  void set_si(const SIVec &si, const params &p);

  ///
  /// \brief set_si for decoding
  /// \param si
  /// \param z_min
  /// \param z_max
  /// \param q
  ///
  void set_si(const SIVec &si, int z_min, int z_max, float q, int l);

  int z_min() const;
  int z_max() const;

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
  int x_binary_length() const;

  bool pqkb(bool negative, int abs_predec, int l, int b, int yk, float &p0,
            float &p1, float &p) const;
  float pqks(int yk) const;

  const Bitplanes &src_bits() const { return m_src_bits; }

  const SymboleVec &src_symboles() const { return m_src_symboles; }
  const SIVec &si_symboles() const { return m_si_symboles; }

  void injectBitplane(uint i, BitVec x);

private:
  struct Pz {
    int zmin, zmax, z0;
    float q0, q1;

    Pz() = default;

    Pz(int pzmin, int pzmax, float pq) : zmin(pzmin), zmax(pzmax), q0(pq) {
      q1 = (zmin == zmax) ? 0.0 : ((1 - q0) / (zmax - zmin));
      z0 = (zmin > 0) ? zmin : (zmax < 0 ? zmax : 0);
    }
  } m_pz_params;

  int m_l;

  SymboleVec m_src_symboles;
  SIVec m_si_symboles;
  Bitplanes m_src_bits;
};
} // namespace icov
