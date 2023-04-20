#pragma once

#include "common_types.h"

namespace icov {
namespace ldpc_codec_layer {

template <class M> class ILDPC_Decoder {
public:
  ILDPC_Decoder() = default;
  virtual ~ILDPC_Decoder() = default;

  virtual void decode(BitVec &o_decoded, int n, int numcode,
                      const std::vector<LLRVal> &llr, Syndrome &syndrome,
                      const M &matrix) = 0;

  virtual void get_llr_order(int i_numcode,
                             std::vector<size_t> &o_idx) const = 0;
};
} // namespace ldpc_codec_layer
} // namespace icov
