#include "CoderFactory.h"

#include "Coder_IMT18.h"
#include "Coder_IMT64.h"
#include "LdpcCoderParameters.h"
#include <boost/program_options.hpp>
#include <memory>
namespace po = boost::program_options;

namespace icov {

p_InteractiveCoder
LdpcCoderFactory::create_coder(const LdpcCoderParameters &args) {
  ICOV_INFO << "Initialization of coder with type " << args.type
            << " and length " << args.codeLength;

  // SHARED PTR -> RESET
  std::shared_ptr<LLRProvider> llrProvider;
  std::shared_ptr<ILDPC_Decoder<AffMatrix>> ldpca_dec;
  std::shared_ptr<ISyndromeManager> syndromeMgr;
  std::shared_ptr<IMatrixProvider<AffMatrix>> mp;

  const std::string &coder_type = args.type;

  if (coder_type == LdpcCoder_IMT18::id) {
    LdpcCoder_IMT18::create(
        llrProvider, ldpca_dec, syndromeMgr, mp,
        dynamic_cast<const LdpcCoder_IMT18::parameters &>(args));
  } else if (coder_type == LdpcCoder_IMT64::id) {
    const auto &p = dynamic_cast<const LdpcCoder_IMT64::parameters &>(args);
    LdpcCoder_IMT64::create(llrProvider, ldpca_dec, syndromeMgr, mp, p);
  } else if (coder_type == "varodayan") {
    throw std::invalid_argument("Coder type not implemented: " + coder_type);
  } else {
    throw std::invalid_argument("Unknown coder type: " + coder_type);
  }

  icov::helpers::assert::not_null(llrProvider.get(),
                                  "LLRProvider was not instantiated");
  icov::helpers::assert::not_null(ldpca_dec.get(),
                                  "Decoder was not instantiated");
  icov::helpers::assert::not_null(mp.get(),
                                  "MatrixProvider was not instantiated");
  icov::helpers::assert::not_null(syndromeMgr.get(),
                                  "SyndromeManager was not instantiated");

  return std::make_unique<InteractiveCoder<AffMatrix>>(
      ldpca_dec, mp, syndromeMgr, llrProvider);
}

void LdpcCoderFactory::create_coder_yuv(const LdpcCoderParameters &args,
                                        p_InteractiveCoder &y_coder,
                                        p_InteractiveCoder &uv_coder) {

  y_coder = LdpcCoderFactory::create_coder(args);
  ICOV_INFO << "Luminance coder instanciated";

  // create parameters for Chroma coder: size of chroma block is 1/4 size of
  // luminance block
  auto args_uv = args.clone();
  args_uv->codeLength /= 4;
  uv_coder = LdpcCoderFactory::create_coder(*args_uv);
  ICOV_INFO << "Chroma coder instanciated";
}

} // namespace icov
