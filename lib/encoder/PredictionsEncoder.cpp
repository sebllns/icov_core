#include "PredictionsEncoder.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"

#include <vector>

icov::encoder::PredictionsEncoder::PredictionsEncoder(
    const LdpcCoderParameters &LDPC_Params,
    std::vector<omnidirectional_layer::IBlockPredictionManager::prediction_t>
        predictions) {
  // populate the prediction coders
  int count = 1;
  for (auto pred : predictions) {
    ICOV_INFO << "Initialize prediction coder " << count << "/"
              << predictions.size();
#pragma omp atomic
    ++count;
    std::unique_ptr<ldpc_codec_layer::AffInteractiveCoder> ycoder, uvcoder;
    ldpc_codec_layer::LdpcCoderFactory::create_coder_yuv(LDPC_Params, ycoder,
                                                         uvcoder);
    m_coders[pred][ycoder->get_n()] = std::move(ycoder);
    m_coders[pred][uvcoder->get_n()] = std::move(uvcoder);
  }

  ICOV_INFO << "Prediction coders initialized";
}

icov::ldpc_codec_layer::AffInteractiveCoder &
icov::encoder::PredictionsEncoder::get_coder(
    icov::omnidirectional_layer::IBlockPredictionManager::prediction_t
        prediction,
    size_t code_size) {

  ICOV_ASSERT_MSG(m_coders.count(prediction) > 0,
                  "Coder not found for the given prediction: " +
                      std::to_string(prediction));

  ICOV_ASSERT_MSG(m_coders.at(prediction).count(code_size) > 0,
                  "Coder not found for the given size: " +
                      std::to_string(code_size));

  return *(m_coders.at(prediction).at(code_size).get());
}

std::vector<size_t> icov::encoder::PredictionsEncoder::getCoderSizes() const {
  std::vector<size_t> sizes;
  for (const auto &p : m_coders.begin()->second)
    sizes.push_back(p.first);
  return sizes;
}
