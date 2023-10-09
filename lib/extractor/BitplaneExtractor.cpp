#include "BitplaneExtractor.h"

#include "FastArray.h"

#include "icov_assert.h"
#include "icov_log.h"
#include "ldpc_codec_layer/QaryDistribution.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "video_layer/ExtractedStreamDebugger.h"
#include "video_layer/encoded_stream.h"
#include "video_layer/extracted_stream.h"
#include "video_layer/icov_stream.h"

#include <algorithm>
#include <boost/algorithm/algorithm.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <string>
#include <sys/types.h>

namespace fs = boost::filesystem;
using namespace icov::extractor;
using SI_SIDE = icov::omnidirectional_layer::BlockPrediction::ePredType;
