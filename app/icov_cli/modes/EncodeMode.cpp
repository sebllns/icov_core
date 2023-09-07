#include "EncodeMode.h"

using namespace icov;
using namespace icov::omnidirectional_layer;
using namespace icov::bitstream;
using namespace icov::encoder;

EncodeMode::EncodeMode(const options::args_t &args)
    : m_encoder(IcovEncoder::params(args), *options::parseLdpcParams(args))
{
}

void EncodeMode::run() {
    m_encoder.run();
}
