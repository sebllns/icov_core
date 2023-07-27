#pragma once

#include "ldpc_codec_layer/BitSymbolProvider.h"
#include "ldpc_codec_layer/interface/IInteractiveCoder.h"
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
 */

class SymbolCoder {
public:
  SymbolCoder(IInteractiveCoder &bitcoder);

  const IInteractiveCoder &bitcoder() const;

  void encode_symboles(std::vector<NumcodeIdxVec> &rates,
                       std::vector<SyndromesVec> &syndromes,
                       const std::vector<SIVec> &si,
                       const QaryDistribution::quantization_params &p,
                       const SymboleVec &src);

  void encode_bitplanes(const BitSymbolProvider &bsp, NumcodeIdxVec &rate,
                        SyndromesVec &stored_synd);

  void encode_bitplane(const BitSymbolProvider &bsp, int bitplane_idx,
                       NumcodeIdx &numcodeIdx, Syndrome &stored_synd,
                       SymboleVec &predec);

  ///
  /// \brief decode_symboles
  /// \param io_bsp BitSymbolProvider that has been previously initialised with
  /// correct parameters (Q, Zmin, Zmax, lx) \param si \param numcodes \param
  /// stored_synd
  ///
  void decode_symboles(BitSymbolProvider &io_bsp, const NumcodeIdxVec &numcodes,
                       const SyndromesVec &stored_synd);

  void decode_bitplane(BitVec &output, int bp_idx, BitSymbolProvider &bsp,
                       const NumcodeIdx &numcode, const Syndrome &stored_synd);

  void init_LLR_bitplane(const BitSymbolProvider &bsp, int bitplane_idx,
                         const BitVec &signs, const SymboleVec &predec);

  int get_n() const;

private:
  IInteractiveCoder &m_bitcoder;
  BitVec m_decoded_bits;
  SymboleVec m_predec;
};
} // namespace ldpc_codec_layer
} // namespace icov
