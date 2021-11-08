#include "CoderFactory.h"

#include "Coder_IMT18.h"
#include "Coder_IMT64.h"

namespace icov
{

template<typename M>
std::shared_ptr<InteractiveCoder<M> > CoderFactory::create_coder(int argc, char *argv[])
{
    if(argc < 5) throw std::invalid_argument("no coder type was given");


    //FILL CP HERE
    CoderParameters cp;
    cp.type = argv[1];
    cp.database = argv[2];
    cp.n = atoi(argv[3]);
    cp.bp_ite = atoi(argv[4]);

    LLRProvider *llrProvider;
    IDecoder<M> *ldpca_dec;
    ISyndromeManager *syndromeMgr;
    IMatrixProvider<M> *mp;

    if(cp.type == "imt18")
    {
        //FILL CP18 HERE
        Coder_IMT18::parameters cp18(cp);

        //INSTANTIATE
        //Coder_IMT18::create()
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
}

//template<>
//std::shared_ptr<InteractiveCoder<AffMatrix> > CoderFactory::create_coder<>(int argc, char *argv[])
//{

//}


}
