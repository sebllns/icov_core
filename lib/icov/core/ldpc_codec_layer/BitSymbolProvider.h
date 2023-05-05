#pragma once

#include "common_types.h"
#include <sys/types.h>

#define Z_STEP 16
#define Z_LIMIT 1024

namespace icov {
namespace ldpc_codec_layer {

/**
 * @brief The BitSymbolProvider class
 *
 * A class that provides methods for computing bits from symbols, and vice
 * versa.
 *
 * BitSymbolProvider is a class that provides a way to convert between
 * bitplanes and symbols. In the context of channel decoding, a set of bitplanes
 * is a 2D array of binary values representing the coded bits at a specific
 * quantization level for a set of pixels (symbols). Each bitplane is an array
 * of bits at a specific position in the binary representation associated to the
 * pixels in a specific quantized and transformed block unit. BitSymbolProvider
 * makes possible the encoding and decoding of these symbols by providing
 * methods to convert the bits to and from symbols.
 *
 * A BitSymbolProvider is a class that provides access to bit codewords used in
 * LDPC (Low Density Parity Check) coding. It is specifically designed to supply
 * bit for LDPC encoding and decoding operations. The BitSymbolProvider
 * class encapsulates the functionality related to managing and retrieving
 * bitplanes from symbols, which are binary values representing information to
 * be encoded or decoded using LDPC codes. It contains methods to access the
 * source bits or symbols, perform operations on them, and provide them to the
 * LDPC coder for encoding or decoding purposes.
 * A bitplane is a way to represent a binary image by dividing it into multiple
 * planes, where each plane represents the value of a single bit. The first
 * bitplane represents sign value ( 1 means value < 0) of the
 * quantized-transformed pixel value. The second bitplane is the most
 * significant bit of each pixel, while the last bitplane represents the least
 * significant bit. Bitplanes are commonly used in image compression algorithms,
 * such as JPEG2000, where each bitplane is compressed separately and then
 * combined to form the final compressed image. By compressing each bitplane
 * separately, the compression algorithm can take advantage of local
 * correlations between neighboring pixels at different bit depths, which can
 * improve the overall compression performance.
 *
 */
class BitSymbolProvider {
public:
  /**
   * @brief The params struct
   *
   * A structure for holding parameters for Pz distribution.
   */
  struct params {
    bool enable_z_quantization;
    std::vector<float> q_values;
  };

  struct PzParams {

  public:
    PzParams();

    /**
     * @brief PzParams constructor
     *
     * Constructs a PzParams object with the specified parameters.
     *
     * @param p_bp_count an unsigned short representing the bitplane count.
     * @param pzmin a short representing the minimum value of Z distribution.
     * @param pzmax a short representing the maximum value of Z distribution.
     * @param pq_quant an unsigned short representing the quantization of q.
     * @param pq a float representing q.
     */
    PzParams(ushort p_bp_count, short pzmin, short pzmax, ushort pq_quant,
             float pq);

    /**
     * @brief getZmin
     *
     * Gets the minimum value of z.
     *
     * @return an int representing the minimum value of z.
     */
    int getZmin() const;

    /**
     * @brief getZmax
     *
     * Gets the maximum value of z.
     *
     * @return an int representing the maximum value of z.
     */
    int getZmax() const;

    /**
     * @brief getZ0
     *
     * Gets the z value associated with q0.
     *
     * @return an int representing the z value associated with q0.
     */
    int getZ0() const;

    /**
     * @brief getQ0
     *
     * Gets the value of q0.
     *
     * @return a float representing the value of q0.
     */
    float getQ0() const;
    int getQ0Quantized() const;
    int getZAmplitude() const;
    float getQ1() const;
    float getPZ(int z) const;
    void setBpCount(ushort newBp_count);
    ushort getBitplaneCount() const;

  private:
    short zmin, zmax, z0;
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
  static PzParams get_pz_params(const SymboleVec &src_symboles,
                                int bitplane_count, const SIVec &si,
                                const params &p);

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
  // float pz(int z) const;

  //  float pz(int zmin, int zmax) const;

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

  const PzParams &pz_params() const;

private:
  PzParams m_pz_params;
  SymboleVec m_src_symboles;
  SIVec m_si_symboles;
  Bitplanes m_src_bits;
};
} // namespace ldpc_codec_layer
} // namespace icov
