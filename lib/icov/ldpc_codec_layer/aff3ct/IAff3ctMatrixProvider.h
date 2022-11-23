#ifndef IAFF3CTMatrixProvider_H
#define IAFF3CTMatrixProvider_H

#include "ldpc_codec_layer/interface/IMatrixProvider.h"
#include <aff3ct.hpp>

using namespace aff3ct;

using AffMatrix = tools::Sparse_matrix;

namespace icov {
class IAff3ctMatrixProvider : public IMatrixProvider<AffMatrix> {
public:
  IAff3ctMatrixProvider() = default;
  virtual ~IAff3ctMatrixProvider() = default;
};

} // namespace icov

#endif // IAFF3CTMatrixProvider_H
