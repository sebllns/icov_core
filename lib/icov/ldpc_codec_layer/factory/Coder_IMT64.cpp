#include "Coder_IMT64.h"
#include "implementation/imt_164/IMT164MatrixProvider.h"
#include "implementation/imt_164/IMT164Syndrome.h"

namespace icov {
const std::string Coder_IMT64::id = "imt164";

void Coder_IMT64::create(
    std::shared_ptr<LLRProvider> &llrProvider,
    std::shared_ptr<IDecoder<AffMatrix>> &ldpca_dec,
    std::shared_ptr<ISyndromeManager> &syndromeMgr,
    std::shared_ptr<IMatrixProvider<AffMatrix>> &mat_provider,
    const parameters &p) {
  llrProvider.reset(new LLRProvider(p.n));

  int min_level = 2;
  int max_level = 64;

  auto numcodes_v = p.getNumcodes(p.n, min_level, max_level);

  std::shared_ptr<IMT164SyndromeManager> imt_synd(new IMT164SyndromeManager(
      p.n, min_level, max_level, numcodes_v.front(), numcodes_v.back()));
  syndromeMgr = imt_synd;

  mat_provider.reset(new IMT164MatrixProvider(imt_synd, p.database));

  // create indexes fromp numcodes
  // last numcode value = where doping starts
  std::for_each(numcodes_v.begin(), numcodes_v.end() - 1,
                [mat_provider, numcodes_v](const int &v) {
                  mat_provider->addNumcode(v - numcodes_v.front(),
                                           v > numcodes_v.back());
                });

  Aff3ctDecoder *dec = new Aff3ctDecoder(mat_provider->getMatrixCount());
  ldpca_dec.reset(dec);

  for (int i = 0; i < mat_provider->getMatrixCount(); i++) {
    int k = mat_provider->getMatrixCount() - i - 1;
    mat_provider->buildMatrix(k);
    dec->init_decoder(k, p.n, p.bp_ite, mat_provider->getMatrix(k));
  }
}
} // namespace icov
