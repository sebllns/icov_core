#include "CoderFactory.h"

#include "Coder_IMT18.h"
#include "Coder_IMT64.h"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

namespace icov
{

std::shared_ptr<InteractiveCoder<AffMatrix> > CoderFactory::create_coder(const boost::program_options::variables_map &args)
{
    //SHARED PTR -> RESET
    std::shared_ptr<LLRProvider> llrProvider;
    std::shared_ptr<IDecoder<AffMatrix>> ldpca_dec;
    std::shared_ptr<ISyndromeManager> syndromeMgr;
    std::shared_ptr<IMatrixProvider<AffMatrix>> mp;

    std::string coder_type = CoderParameters::parseType(args);

    if(coder_type == Coder_IMT18::id)
    {
        Coder_IMT18::create(llrProvider, ldpca_dec, syndromeMgr, mp, Coder_IMT18::parameters (args));
    }
    else if(coder_type == Coder_IMT64::id)
    {
        Coder_IMT64::create(llrProvider, ldpca_dec, syndromeMgr, mp, Coder_IMT64::parameters(args));
    }
    else if(coder_type == "varodayan")
    {

    }
    else
    {
        throw std::invalid_argument("Unknown coder type: " + coder_type);
    }

    if(!llrProvider)
    {
        throw std::runtime_error("LLRProvider was not instantiated");
    }

    if(!ldpca_dec)
    {
        throw std::runtime_error("Decoder was not instantiated");
    }

    if(!mp)
    {
        throw std::runtime_error("MatrixProvider was not instantiated");
    }

    if(!syndromeMgr)
    {
        throw std::runtime_error("SyndromeManager was not instantiated");
    }

    std::cout<<"Initialization of coder with type = "<<coder_type<<std::endl;

    return std::make_shared<InteractiveCoder<AffMatrix>>(ldpca_dec, mp, syndromeMgr, llrProvider);
}

}
