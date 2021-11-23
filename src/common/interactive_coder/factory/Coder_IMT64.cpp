#include "Coder_IMT64.h"
#include "interactive_coder/implementation/imt_164/IMT164MatrixProvider.h"
#include "interactive_coder/implementation/imt_164/IMT164Syndrome.h"

namespace icov
{
const std::string Coder_IMT64::id = "imt164";

void Coder_IMT64::create(std::shared_ptr<LLRProvider> &llrProvider, std::shared_ptr<IDecoder<AffMatrix>> &ldpca_dec, std::shared_ptr<ISyndromeManager> &syndromeMgr,  std::shared_ptr<IMatrixProvider<AffMatrix>> &mat_provider, const parameters &p)
{
    llrProvider.reset(new LLRProvider(p.n));

    int min_level = 2;
    int max_level = 64;

    std::shared_ptr<IMT164SyndromeManager> imt_synd(new IMT164SyndromeManager(p.n, min_level, max_level));
    syndromeMgr = imt_synd;

    mat_provider.reset(new IMT164MatrixProvider(imt_synd, p.database));

    int n =  imt_synd->get_n();
    int n_min = imt_synd->get_m_min();
    int n_max = imt_synd->get_m_max();

    int pow = std::log2f(p.n);

    std::vector<encode_parameters> encode_params(3);
    encode_params[0].start_code = 0;
    encode_params[0].stop_code = n_max - n_min;
    // encode_params[0].step = std::floor(4*pow/8);
    encode_params[0].step = std::ceil(pow/8) * 4;

    encode_params[1].start_code = encode_params[0].stop_code;
    encode_params[1].stop_code = n - n_min;
    // encode_params[1].step =  std::floor(5*pow/8);
    encode_params[1].step = encode_params[0].step;

    encode_params[2].start_code = encode_params[1].stop_code;
    encode_params[2].stop_code = 3*n_max - n_min + 1;
    // encode_params[2].step =  std::floor(9*pow/8);
    encode_params[2].step = 2 * encode_params[1].step;

    icov::index_t numcode = 0;

//    for(auto &param: encode_params)
//    {
//        for(icov::index_t c = param.start_code; c < param.stop_code; c+= param.step)
//        {
//            if(numcode == 0 || c > numcode)
//            {
//                numcode = c;
//                mat_provider->addNumcode(numcode, numcode > (n_max-n_min));
//            }
//        }
//    }

    for(int i = 0 ; i < encode_params[2].stop_code; i++)
    {
        mat_provider->addNumcode(i, i > (n_max-n_min));
    }


    Aff3ctDecoder *dec = new Aff3ctDecoder(mat_provider->getMatrixCount());
    ldpca_dec.reset(dec);

    for(int i = 0 ; i < mat_provider->getMatrixCount() ; i++)
    {
        int k = mat_provider->getMatrixCount() - i - 1;
        mat_provider->buildMatrix(k);
        dec->init_decoder(k, p.n, p.bp_ite, mat_provider->getMatrix(k));
    }
}
}
