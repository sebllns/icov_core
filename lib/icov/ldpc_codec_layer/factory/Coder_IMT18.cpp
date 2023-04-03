#include "Coder_IMT18.h"
#include "ldpc_codec_layer/implementation/imt/IMTMatrixProvider.h"
#include "ldpc_codec_layer/implementation/imt/IMTSyndrome.h"

namespace icov {
const std::string LdpcCoder_IMT18::id = "imt18";

void LdpcCoder_IMT18::create(
    std::shared_ptr<LLRProvider> &llrProvider,
    std::shared_ptr<ILDPC_Decoder<AffMatrix>> &ldpca_dec,
    std::shared_ptr<ISyndromeManager> &syndromeMgr,
    std::shared_ptr<IMatrixProvider<AffMatrix>> &mat_provider,
    const parameters &p) {
  llrProvider.reset(new LLRProvider(p.codeLength));

  std::shared_ptr<IMTSyndromeManager> imt_synd(
      new IMTSyndromeManager(p.codeLength));
  syndromeMgr = imt_synd;

  mat_provider.reset(new IMTMatrixProvider(imt_synd, p.database));

  int n = imt_synd->get_n();
  int n_8 = n / 8;
  int n_2 = n / 2;

  int pow = std::log2f(p.codeLength);

  std::vector<encode_parameters> encode_params(3);
  encode_params[0].start_code = 0;
  encode_params[0].stop_code = n_2 - n_8;
  // encode_params[0].step = std::floor(4*pow/8);
  encode_params[0].step = std::ceil(pow / 8) * 4;

  encode_params[1].start_code = encode_params[0].stop_code;
  encode_params[1].stop_code = n - n_8;
  // encode_params[1].step =  std::floor(5*pow/8);
  encode_params[1].step = encode_params[0].step;

  encode_params[2].start_code = encode_params[1].stop_code;
  encode_params[2].stop_code = 3 * n_2 - n_8 + 1;
  // encode_params[2].step =  std::floor(9*pow/8);
  encode_params[2].step = 2 * encode_params[1].step;

  icov::index_t numcode = 0;

  for (auto &param : encode_params) {
    for (icov::index_t c = param.start_code; c < param.stop_code;
         c += param.step) {
      if (numcode == 0 || c > numcode) {
        numcode = c;
        mat_provider->addNumcode(numcode, numcode > (n_2 - n_8));
      }
    }
  }

  Aff3ctDecoder *dec = new Aff3ctDecoder(mat_provider->getMatrixCount());
  ldpca_dec.reset(dec);

  for (int i = 0; i < mat_provider->getMatrixCount(); i++) {
    mat_provider->buildMatrix(i);
    dec->init_decoder(i, p.codeLength, p.beliefPropagationInterations,
                      mat_provider->getMatrix(i));
  }
}
} // namespace icov
