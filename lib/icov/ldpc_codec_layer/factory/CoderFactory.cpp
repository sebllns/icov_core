#include "CoderFactory.h"

#include "Coder_IMT18.h"
#include "Coder_IMT64.h"
#include "IcovCoder.h"
#include "icov_program_options.h"

#include <boost/program_options.hpp>
#include <memory>
namespace po = boost::program_options;

namespace icov {

std::unique_ptr<LdpcCoderParameters> LdpcCoderFactory::parse_coder_parameters(
    const boost::program_options::variables_map &args) {
  std::string coder_type = LdpcCoderParameters::parseType(args);

  if (coder_type == LdpcCoder_IMT18::id) {
    return std::make_unique<LdpcCoder_IMT18::parameters>(args);
  } else if (coder_type == LdpcCoder_IMT64::id) {
    return std::make_unique<LdpcCoder_IMT64::parameters>(args);
  } else if (coder_type == "varodayan") {
    throw std::invalid_argument("Coder type not implemented: " + coder_type);
  } else {
    throw std::invalid_argument("Unknown coder type: " + coder_type);
  }
}

p_InteractiveCoder LdpcCoderFactory::create_coder(const LdpcCoderParameters &args) {
  ICOV_INFO << "Initialization of coder with type " << args.type
            << " and length " << args.n << " >>>> " << args.database;

  // SHARED PTR -> RESET
  std::shared_ptr<LLRProvider> llrProvider;
  std::shared_ptr<ILDPC_Decoder<AffMatrix>> ldpca_dec;
  std::shared_ptr<ISyndromeManager> syndromeMgr;
  std::shared_ptr<IMatrixProvider<AffMatrix>> mp;

  const std::string &coder_type = args.type;

  if (coder_type == LdpcCoder_IMT18::id) {
    LdpcCoder_IMT18::create(llrProvider, ldpca_dec, syndromeMgr, mp,
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

p_InteractiveCoder
LdpcCoderFactory::create_coder(const boost::program_options::variables_map &args) {
  auto p = parse_coder_parameters(args);
  return create_coder(*p);
}

void LdpcCoderFactory::create_coder_yuv(const LdpcCoderParameters &args,
                                    p_InteractiveCoder &y_coder,
                                    p_InteractiveCoder &uv_coder) {

  y_coder = LdpcCoderFactory::create_coder(args);
  ICOV_INFO << "Luminance coder instanciated";

  // create parameters for Chroma coder: size of chroma block is 1/4 size of
  // luminance block
  auto args_uv = args.clone();
  args_uv->n /= 4;
  // setCodeLength(args_uv, args[OPT_CODE_LENGTH].as<int>() / 4);
  uv_coder = LdpcCoderFactory::create_coder(*args_uv);
  ICOV_INFO << "Chroma coder instanciated";
}

void LdpcCoderFactory::create_coder_yuv(
    const boost::program_options::variables_map &args,
    p_InteractiveCoder &y_coder, p_InteractiveCoder &uv_coder) {
  auto p = parse_coder_parameters(args);
  return create_coder_yuv(*p, y_coder, uv_coder);
}

} // namespace icov
