#include "ChannelExtractor.h"

#include "helpers.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "omnidirectional_layer/BlockPrediction.h"
#include <algorithm>
#include <boost/algorithm/algorithm.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

namespace fs = boost::filesystem;
using namespace icov::extractor;
using SI_SIDE = BlockPrediction::predType;

ChannelExtractor::ChannelExtractor(/*const params &p,*/
                                   std::shared_ptr<InputIcov> &icovFile,
                                   const CoderParameters &LDPC_params)
    : /*m_params(p),*/ m_ldpc_coder(
          icov::CoderFactory::create_coder(LDPC_params)),
      m_icovFile(icovFile) {}

ChannelExtractor::~ChannelExtractor() {}

void ChannelExtractor::extract_access_block(const Block &b,
                                            icov::byte *syndrome_buf) {}

void ChannelExtractor::extract_block(const Block &b,
                                   BlockPrediction::predType block_prediction,
                                   BlockChannelDecoderParams &extractedParams,
                                   byte *syndrome_buf) {
  ICOV_ASSERT_MSG(block_prediction != BlockPrediction::predType::UNKNOWN &&
                      block_prediction != BlockPrediction::predType::NONE,
                  "Prediction unknown");
}
