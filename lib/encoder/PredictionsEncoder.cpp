#include "PredictionsEncoder.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "tools/DebugTools.h"

icov::encoder::PredictionsEncoder::PredictionsEncoder(
    const boost::program_options::variables_map &program_args,
    std::vector<uint16_t> predictions) {
  // populate the prediction coders
  int count = 1;
  for (auto pred : predictions) {
    ICOV_INFO << "Initialize prediction coder " << count << "/"
                   << predictions.size();
#pragma omp atomic
    ++count;
    std::unique_ptr<AffInteractiveCoder> ycoder, uvcoder;
    icov::CoderFactory::create_coder_yuv(program_args, ycoder, uvcoder);
    m_coders[pred][ycoder->get_n()] = std::move(ycoder);
    m_coders[pred][uvcoder->get_n()] = std::move(uvcoder);
  }

  ICOV_INFO << "Prediction coders initialized";
}
