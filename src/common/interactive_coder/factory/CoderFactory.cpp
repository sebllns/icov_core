#include "CoderFactory.h"

#include "Coder_IMT18.h"
#include "Coder_IMT64.h"

namespace icov
{

template<typename M>
std::shared_ptr<InteractiveCoder<M> > CoderFactory::create_coder(int argc, char *argv[])
{
    throw std::runtime_error("CoderFactory does not implement the coder instanciation for matrix of type " + std::string(typeid (M).name()));
}

template<>
std::shared_ptr<InteractiveCoder<AffMatrix> > CoderFactory::create_coder<>(int argc, char *argv[])
{
    if(argc < 5) throw std::invalid_argument("no coder type was given");


    //FILL CP HERE
    CoderParameters cp;
    cp.type = argv[1];
    cp.database = argv[2];
    cp.n = atoi(argv[3]);
    cp.bp_ite = atoi(argv[4]);

    LLRProvider *llrProvider = nullptr;
    IDecoder<AffMatrix> *ldpca_dec = nullptr;
    ISyndromeManager *syndromeMgr = nullptr;
    IMatrixProvider<AffMatrix> *mp = nullptr;

    if(cp.type == "imt18")
    {

        //FILL CP18 HERE
        Coder_IMT18::parameters cp18(cp);

        //INSTANTIATE
        Coder_IMT18::create(llrProvider, ldpca_dec, syndromeMgr, mp, cp18);
    }
    else if(cp.type == "imt64")
    {

    }
    else if(cp.type == "varodayan")
    {

    }
    else
    {
        throw std::invalid_argument("Unknown coder type: " + cp.type);
    }

    std::cout<<"type of coder = "<<cp.type<<std::endl;
    return std::shared_ptr<InteractiveCoder<AffMatrix>>(new InteractiveCoder<AffMatrix>(ldpca_dec, mp, syndromeMgr, llrProvider));
}


}
