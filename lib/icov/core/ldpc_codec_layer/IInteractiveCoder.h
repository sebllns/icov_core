#pragma once

#include "common.h"

namespace icov {

class IInteractiveCoder {
public:
  static struct t_params { bool CheckBitplaneEncoding; } params;
  virtual ~IInteractiveCoder() = default;
  virtual size_t get_n() const = 0;
};

} // namespace icov
