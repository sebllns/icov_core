#include "ldpc_codec_layer/InteractiveCoder.h"
#include <Tools/Algo/Matrix/Sparse_matrix/Sparse_matrix.hpp>
#include <memory>

using AffMatrix = aff3ct::tools::Sparse_matrix;
using AffInteractiveCoder = icov::InteractiveCoder<AffMatrix>;
using p_InteractiveCoder = std::unique_ptr<icov::InteractiveCoder<AffMatrix>>;
