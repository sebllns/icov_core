#include "BlockExtractor.h"

#include "extractor/ChannelExtractor.h"
#include "icov_io.h"
#include "icov_log.h"
#include "icov_types.h"
#include "omnidirectional_layer/interface/IBlockProcessor.h"
#include "video_layer/ExtractedStreamDebugger.h"
#include "video_layer/VideoMetrics.h"
#include "video_layer/encoded_stream.h"
#include "video_layer/extracted_stream.h"
#include "video_layer/icov_stream.h"

#include <algorithm>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <iterator>
#include <memory>

using namespace icov;
using namespace icov::extractor;
using namespace icov::omnidirectional_layer;
using namespace icov::bitstream;

icov::extractor::BlockExtractor::BlockExtractor(
    std::shared_ptr<InputIcov> &icovFile, const params &p,
    const LdpcCoderParameters &coder_params,
    const bitstream::sp_IcovExtractedBitstream &extraction)
    : omnidirectional_layer::IBlockProcessor(), m_icovFile(icovFile),
      m_extractionBitstream(extraction), m_channelCount(p.icu_sizes.size()),
      m_params(p) {
  this->setName("Extractor");

  bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingFrameID(0);

  int8b compID = 0;
  for (int n : p.icu_sizes) {
    auto channel_params = coder_params.clone();
    channel_params->codeLength = n;
    ChannelExtractor::params channel_extractor_params{.CompID = compID};
    m_channelExtractor.push_back(std::make_unique<ChannelExtractor>(
        channel_extractor_params, m_icovFile, m_extractionBitstream,
        *channel_params));
    compID++;
  }
}

icov::extractor::BlockExtractor::~BlockExtractor() {}

void icov::extractor::BlockExtractor::fill_metrics(
    icov::omnidirectional_layer::IBlockPredictionManager::prediction_t
        prediction,
    const Block &b) {
  icov::video_layer::VideoMetrics::get_mutable_instance().metrics.resize(
      b.parent()->blockCount());
  icov::video_layer::VideoMetrics::get_mutable_instance().getBlock(b).reset();
  icov::video_layer::VideoMetrics::get_mutable_instance().getBlock(b).id =
      b.getIdSelf();
  icov::video_layer::VideoMetrics::get_mutable_instance()
      .getBlock(b)
      .prediction = prediction;
  icov::video_layer::VideoMetrics::get_mutable_instance()
      .visible_blocks.push_back(b.getIdSelf());

  // extract mse
  float mse = 0.;
  m_icovFile->read(bitstream::eIcovStream::METRICS, mse);
  icov::video_layer::VideoMetrics::get_mutable_instance().getBlock(b).mse = mse;
}

void icov::extractor::BlockExtractor::_processBlock(
    const request_t &r, const Block &b,
    icov::omnidirectional_layer::IBlockPredictionManager::prediction_t
        prediction,
    frame_t previouslyDecoded) {

  bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingBlockID(
      b.parent()->getBlockIndexPos(b.getIdSelf()));

  icov::omnidirectional_layer::BlockPrediction::ePredType block_prediction =
      icov::omnidirectional_layer::BlockPrediction::toPredType(prediction);

  ICOV_DEBUG << "START EXTRACT BLOCK " << b.getIdSelf()
             << " WITH INTRA PREDICTION "
             << omnidirectional_layer::BlockPrediction::toString(
                    block_prediction);

  bool is_access_block =
      (block_prediction ==
       icov::omnidirectional_layer::BlockPrediction::ePredType::NONE);

  std::cout << "SELECT BLOCK " << b.getIdSelf() << std::endl;
  m_icovFile->selectBlock(b.getIdSelf());

  if (true || previouslyDecoded != r.frameID) {
    // get stored syndrome size per bitplane
    std::cout << "extract_syndrome_sizes" << std::endl;
    for (auto &ch : m_channelExtractor) {
      ch->extract_stored_syndrome_size(is_access_block);
    }

    //    if (is_access_block) {
    //      fs::path ab_path = m_icovFile->getPath() / std::to_string(r.frameID)
    //      /
    //                         (std::to_string(b.getIdSelf()) + ".tiff");
    //      ICOV_DEBUG << "extract access block " << ab_path;
    //      ushort filesize = fs::file_size(ab_path);
    //      std::ifstream fin(ab_path, std::ios::binary);
    //      char buf[filesize];
    //      fin.read(buf, filesize);
    //      // ICOV_DEBUG << icov::io::make_hex_string(buf, buf + filesize,
    //      true,
    //      // true);
    //    }

    if (!is_access_block) {

      // pass access block syndrome  per bitplane
      if (b.isAccessBlock()) {
        ICOV_TRACE << "PASS ACCESS BLOCK";
        ChannelExtractor::resetSyndromePadding();
        for (auto &ch : m_channelExtractor) {
          ch->pass_access_block();
        }
      }

      bool check_inter =
          r.frameID > 0 && (r.frameID == (previouslyDecoded + 1));
      BitLength inter_length = std::numeric_limits<BitLength>::infinity();
      BitLength intra_length = 0;

      if (check_inter) {
        // extract inter-prediction paramaters and calcultate its bitrate
        for (auto &ch : m_channelExtractor) {
          ch->extractPredParams(S_INTER);
          inter_length += ch->predParams().at(S_INTER).getSyndromeLength();
        }

        ICOV_DEBUG << "INTER LENGTH " << inter_length;
      }

      // jump to selected intra prediction parameters
      int pred_idx = bitstream::getPredictionOrder(block_prediction) -
                     (check_inter ? 1 : 0);

      for (int i = 0; i < pred_idx; ++i) {
        for (auto &ch : m_channelExtractor)
          ch->pass_prediction();
      }

      // extract intra-prediction bitrate and calcultate its bitrate
      for (auto &ch : m_channelExtractor) {
        ch->extractPredParams(S_INTRA);
        intra_length += ch->predParams().at(S_INTRA).getSyndromeLength();
      }

      ICOV_DEBUG << "INTRA LENGTH " << intra_length;

      // select prediction with minimal bitrate
      if (check_inter && (inter_length <= intra_length)) {
        block_prediction =
            icov::omnidirectional_layer::BlockPrediction::ePredType::INTER;

        ICOV_DEBUG << "BLOCK " << b.getIdSelf() << " ON FRAME " << r.frameID
                   << " SELECT INTER PREDICTION TO SAVE "
                   << (intra_length - inter_length) << " BITS (SAVE "
                   << (100.0 * (intra_length - inter_length)) / intra_length
                   << "% OVER INTRA)";
      }

      ICOV_DEBUG << "SELECT PREDICTION "
                 << omnidirectional_layer::BlockPrediction::toString(
                        block_prediction);
    }

    // send informations to the debugger
    bitstream::VideoStreamDebugger::get_mutable_instance()
        .setDebuggingPredictionID(block_prediction);

    ChannelExtractor::resetSyndromePadding();
    int c = 0;
    for (auto &ch : m_channelExtractor) {
      ICOV_TRACE << "START EXTRACT CHANNEL " << c;
      ch->extract_block_syndrome(b, block_prediction);
      ICOV_TRACE << "END EXTRACT CHANNEL " << c;
    }
  }

  ICOV_DEBUG << "END EXTRACT BLOCK " << b.getIdSelf() << " WITH PREDICTION "
             << omnidirectional_layer::BlockPrediction::toString(
                    block_prediction);

  fill_metrics(prediction, b);
}

void icov::extractor::BlockExtractor::_beforeOrderingLoop(const request_t &r) {
  icov::video_layer::VideoMetrics::get_mutable_instance()
      .visible_blocks.clear();
  m_icovFile->setFrame(r.frameID);
}

void icov::extractor::BlockExtractor::_afterOrderingLoop(const request_t &r) {}
