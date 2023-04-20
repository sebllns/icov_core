#include "ldpc_codec_layer/InteractiveCoder.h"
#include <Tools/Algo/Matrix/Sparse_matrix/Sparse_matrix.hpp>
#include <memory>

namespace icov {
namespace ldpc_codec_layer {
using AffMatrix = aff3ct::tools::Sparse_matrix;
using AffInteractiveCoder = InteractiveCoder<AffMatrix>;
using p_InteractiveCoder = std::unique_ptr<InteractiveCoder<AffMatrix>>;
} // namespace ldpc_codec_layer
} // namespace icov
