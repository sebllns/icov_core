#ifndef IAFF3CTMatrixProvider_H
#define IAFF3CTMatrixProvider_H

#include "Tools/Algo/Matrix/Sparse_matrix/Sparse_matrix.hpp"
#include "ldpc_codec_layer/interface/IMatrixProvider.h"

namespace icov {
namespace ldpc_codec_layer {

using AffMatrix = aff3ct::tools::Sparse_matrix;

class IAff3ctMatrixProvider : public IMatrixProvider<AffMatrix> {
public:
  IAff3ctMatrixProvider() = default;
  virtual ~IAff3ctMatrixProvider() = default;
};

} // namespace ldpc_codec_layer
} // namespace icov

#endif // IAFF3CTMatrixProvider_H
