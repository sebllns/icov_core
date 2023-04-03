#include "Coder_IMT64.h"
#include "implementation/imt_164/IMT164Syndrome.h"
#include "tools/DebugTools.h"
#include <memory>

namespace icov {
const std::string LdpcCoder_IMT64::id = "imt164";
// std::map<std::string, std::shared_ptr<IMatrixProvider<AffMatrix>>>
//    LdpcCoder_IMT64::matrix_provider_pool =
//        std::map<std::string, std::shared_ptr<IMatrixProvider<AffMatrix>>>();

std::map<std::string, std::shared_ptr<IMT164MatrixProvider>>
    LdpcCoder_IMT64::matrix_provider_pool =
        std::map<std::string, std::shared_ptr<IMT164MatrixProvider>>();

void LdpcCoder_IMT64::create(
    std::shared_ptr<LLRProvider> &llrProvider,
    std::shared_ptr<ILDPC_Decoder<AffMatrix>> &ldpca_dec,
    std::shared_ptr<ISyndromeManager> &syndromeMgr,
    std::shared_ptr<IMatrixProvider<AffMatrix>> &mat_provider,
    const parameters &p) {

  llrProvider.reset(new LLRProvider(p.codeLength));

  int min_level = 2;
  int max_level = 64;

  auto numcodes_v = p.getNumcodes(p.codeLength, min_level, max_level);

  IMT164SyndromeTools synd_tools(p.codeLength, numcodes_v.front(),
                                 numcodes_v.back());

  std::string root_matrix =
      IMT164MatrixProvider::get_matrix_root(p.database, synd_tools.get_n());

  bool init_matrix = false;

  if (matrix_provider_pool.count(root_matrix) == 0) {
    init_matrix = true;
    matrix_provider_pool.insert_or_assign(
        root_matrix, std::make_shared<IMT164MatrixProvider>(
                         p.database, min_level, max_level, synd_tools));
  }

  mat_provider = matrix_provider_pool.at(root_matrix);

  // create indexes fromp numcodes
  // last numcode value = where doping starts
  if (init_matrix)
    std::for_each(numcodes_v.begin(), numcodes_v.end() - 1,
                  [mat_provider, numcodes_v](const int &v) {
                    mat_provider->addNumcode(v - numcodes_v.front(),
                                             v > numcodes_v.back());
                  });

  Aff3ctDecoder *dec = new Aff3ctDecoder(mat_provider->getMatrixCount());
  ldpca_dec.reset(dec);

  for (int i = 0; i < mat_provider->getMatrixCount(); i++) {
    int k = mat_provider->getMatrixCount() - i - 1;
    if (init_matrix)
      mat_provider->buildMatrix(k);
    dec->init_decoder(k, p.codeLength, p.beliefPropagationInterations,
                      mat_provider->getMatrix(k));
  }

  std::shared_ptr<IMT164SyndromeManager> imt_synd =
      std::make_shared<IMT164SyndromeManager>(
          synd_tools, matrix_provider_pool.at(root_matrix));
  syndromeMgr = imt_synd;
}
} // namespace icov
