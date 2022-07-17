#include "EncodeMode.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "tools/DebugTools.h"
#include "encoder/FakeEncoder.h"
#include "encoder/Encoder.h"

void EncodeMode::AddProgramOptions(boost::program_options::options_description &desc)
{
}

EncodeMode::EncodeMode(const po::variables_map &args):
   m_encoder(std::make_unique<Encoder>())
{
    CoderFactory::create_coder_yuv(args, m_ycoder, m_uvcoder);

}

void EncodeMode::run()
{
    std::cout<<"encode\n";


}

