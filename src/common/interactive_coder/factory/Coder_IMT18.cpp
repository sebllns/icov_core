#include "Coder_IMT18.h"
namespace icov
{
const std::string Coder_IMT18::id = "imt18";

void Coder_IMT18::create(LLRProvider *llrProvider, IDecoder<AffMatrix> *ldpca_dec, ISyndromeManager *syndromeMgr, IMatrixProvider<AffMatrix> *mat_provider, const parameters &p)
{
    llrProvider = new LLRProvider(p.n);
    ldpca_dec = new Aff3ctDecoder(p.n);

    IMTSyndromeManager* imt_synd = new IMTSyndromeManager(p.n);
    syndromeMgr = imt_synd;

    mat_provider = new IMTMatrixProvider(imt_synd, p.database);

    int n =  imt_synd->get_n();
    int n_8 = n/8;
    int n_2 = n/2;

    int pow = std::log2f(p.n);

    std::vector<encode_parameters> encode_params(3);
    encode_params[0].start_code = 0;
    encode_params[0].stop_code = n_2 - n_8;
    // encode_params[0].step = std::floor(4*pow/8);
    encode_params[0].step = std::ceil(pow/8) * 4;

    encode_params[1].start_code = encode_params[0].stop_code;
    encode_params[1].stop_code = n - n_8;
    // encode_params[1].step =  std::floor(5*pow/8);
    encode_params[1].step = encode_params[0].step;

    encode_params[2].start_code = encode_params[1].stop_code;
    encode_params[2].stop_code = 3*n_2 - n_8 + 1;
    // encode_params[2].step =  std::floor(9*pow/8);
    encode_params[2].step = 2 * encode_params[1].step;

    icov::index_t numcode = -1;

    for(auto &param: encode_params)
    {
        for(icov::index_t c = param.start_code; c < param.stop_code; c+= param.step)
        {
            if(c > numcode)
            {
                numcode = c;
                mat_provider->addNumcode(numcode);
            }
        }
    }

    if(numcode < encode_params[2].stop_code) mat_provider->addNumcode(encode_params[2].stop_code);
}

}
