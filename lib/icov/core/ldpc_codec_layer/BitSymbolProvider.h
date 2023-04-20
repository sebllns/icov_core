#pragma once

#include "common_types.h"
#include <sys/types.h>

#define Z_STEP 16
#define Z_LIMIT 1024

namespace icov {
namespace ldpc_codec_layer {

class BitSymbolProvider {
public:
  struct params {
    bool enable_z_quantization;
    std::vector<float> q_values;
  };

  struct PzParams {

  public:
    PzParams() = default;
    PzParams(int p_bp_count, int pzmin, int pzmax, int pq_quant, float pq)
        : zmin(pzmin), zmax(pzmax), q0_quantized(pq_quant),
          bp_count(p_bp_count), q0(pq) {}

    int getZmin() const;
    int getZmax() const;
    int getZ0() const;
    float getQ0() const;
    int getQ0Quantized() const;

    int getZAmplitude() const;
    float getQ1() const;

    void setBpCount(ushort newBp_count);

    ushort getBitplaneCount() const;

  private:
    ushort zmin, zmax, z0;
    ushort q0_quantized, bp_count;
    float q0;
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

  ///
  /// \brief set_si for encode mode: set SI and update Pz distribution
  /// parameters \param si \param enable_z_quantization \param q_values
  ///
  static PzParams get_pz(const SymboleVec &src_symboles, int bitplane_count,
                         const SIVec &si, const params &p);

  void inject_bitplane(SymboleVec &output, const BitVec &input, int bp_shift);

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

  void set_si(const SIVec &si, const params &p);

  ///
  /// \brief set_si for decoding
  /// \param si
  /// \param z_min
  /// \param z_max
  /// \param q
  ///
  void set_si(const SIVec &si, const PzParams &pz);

  //  int z_min() const;
  //  int z_max() const;

  //  float y_min() const;
  //  float y_max() const;

  //  int q_idx() const;

  //  // q
  //  float q0() const;

  //  // q'
  //  float q1() const;

  //  // z associated with q0 = min(|zk|)
  //  int z0() const;

  //..0..0 [q'..q' q q'..q'] 0..0..
  float pz(int z) const;

  float pz(int zmin, int zmax) const;

  // L
  int bitplane_count() const;
  int code_length() const;

  bool pqkb(bool negative, int abs_predec, int l, int b, int yk, float &p0,
            float &p1, float &p) const;
  float pqks(int yk) const;

  const Bitplanes &src_bits() const { return m_src_bits; }

  const SymboleVec &src_symboles() const { return m_src_symboles; }
  const SIVec &si_symboles() const { return m_si_symboles; }

  void injectBitplane(uint i, BitVec x);

  const PzParams pz_params() const;

private:
  PzParams m_pz_params;
  SymboleVec m_src_symboles;
  SIVec m_si_symboles;
  Bitplanes m_src_bits;
};
} // namespace ldpc_codec_layer
} // namespace icov
