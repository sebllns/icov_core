#include "CoderFactory.h"

#include "Coder_IMT18.h"
#include "Coder_IMT64.h"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

namespace icov {

p_InteractiveCoder
CoderFactory::create_coder(const boost::program_options::variables_map &args) {
  // SHARED PTR -> RESET
  std::shared_ptr<LLRProvider> llrProvider;
  std::shared_ptr<IDecoder<AffMatrix>> ldpca_dec;
  std::shared_ptr<ISyndromeManager> syndromeMgr;
  std::shared_ptr<IMatrixProvider<AffMatrix>> mp;

  std::string coder_type = CoderParameters::parseType(args);

  if (coder_type == Coder_IMT18::id) {
    Coder_IMT18::create(llrProvider, ldpca_dec, syndromeMgr, mp,
                        Coder_IMT18::parameters(args));
  } else if (coder_type == Coder_IMT64::id) {
    Coder_IMT64::create(llrProvider, ldpca_dec, syndromeMgr, mp,
                        Coder_IMT64::parameters(args));
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

  ICOV_LOG(info) << "Initialization of coder with type " << coder_type
                 << " and length " << syndromeMgr->get_n();

  return std::make_unique<InteractiveCoder<AffMatrix>>(
      ldpca_dec, mp, syndromeMgr, llrProvider);
}

void CoderFactory::create_coder_yuv(
    const boost::program_options::variables_map &args,
    p_InteractiveCoder &y_coder, p_InteractiveCoder &uv_coder) {
  boost::program_options::variables_map args_uv(args);
  boost::any uv_length(args[OPT_CODE_LENGTH].as<int>() / 4);
  auto code_length_option = args_uv[OPT_CODE_LENGTH];
  code_length_option.value() = uv_length;
  args_uv.insert_or_assign(OPT_CODE_LENGTH, code_length_option);

  y_coder = CoderFactory::create_coder(args);
  uv_coder = CoderFactory::create_coder(args_uv);
}

} // namespace icov
