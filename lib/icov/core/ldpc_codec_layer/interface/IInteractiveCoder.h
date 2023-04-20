#pragma once

#include "common_types.h"
#include <cstddef>

namespace icov {
namespace ldpc_codec_layer {

class IInteractiveCoder {
public:
  static struct t_params { bool CheckBitplaneEncoding; } params;
  virtual ~IInteractiveCoder() = default;
  virtual size_t get_n() const = 0;
  virtual int get_m(const NumcodeIdx &numcodeIdx) const = 0;
  virtual Rate get_r(const NumcodeIdx &numcodeIdx) const = 0;
  virtual Rate get_r(int m, int n) const = 0;
};
} // namespace ldpc_codec_layer
} // namespace icov
