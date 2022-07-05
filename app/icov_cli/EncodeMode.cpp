#include "EncodeMode.h"

#include "ldpc_codec/factory/CoderFactory.h"

#include "tools/DebugTools.h"

void EncodeMode::AddProgramOptions(boost::program_options::options_description &desc)
{

}

EncodeMode::EncodeMode(const po::variables_map &args)
{
    CoderFactory::create_coder_yuv(args, m_ycoder, m_uvcoder);

}

void EncodeMode::run()
{
    std::cout<<"encode\n";


}

