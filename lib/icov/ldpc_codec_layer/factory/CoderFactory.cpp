#include "CoderFactory.h"

#include "Coder_IMT18.h"
#include "Coder_IMT64.h"
#include "IcovCoder.h"
#include "icov_program_options.h"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

namespace icov {

CoderParameters CoderFactory::parse_coder_parameters(
    const boost::program_options::variables_map &args) {
  std::string coder_type = CoderParameters::parseType(args);

  if (coder_type == Coder_IMT18::id) {
    return Coder_IMT18::parameters(args);
  } else if (coder_type == Coder_IMT64::id) {
    return Coder_IMT64::parameters(args);
  } else if (coder_type == "varodayan") {
    throw std::invalid_argument("Coder type not implemented: " + coder_type);
  } else {
    throw std::invalid_argument("Unknown coder type: " + coder_type);
  }
}

p_InteractiveCoder CoderFactory::create_coder(const CoderParameters &args) {
  // SHARED PTR -> RESET
  std::shared_ptr<LLRProvider> llrProvider;
  std::shared_ptr<IDecoder<AffMatrix>> ldpca_dec;
  std::shared_ptr<ISyndromeManager> syndromeMgr;
  std::shared_ptr<IMatrixProvider<AffMatrix>> mp;

  const std::string coder_type = args.type;

  if (coder_type == Coder_IMT18::id) {
    Coder_IMT18::create(llrProvider, ldpca_dec, syndromeMgr, mp,
                        static_cast<const Coder_IMT18::parameters &>(args));
  } else if (coder_type == Coder_IMT64::id) {
    Coder_IMT64::create(llrProvider, ldpca_dec, syndromeMgr, mp,
                        static_cast<const Coder_IMT64::parameters &>(args));
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

  ICOV_INFO << "Initialization of coder with type " << coder_type
            << " and length " << syndromeMgr->get_n();

  return std::make_unique<InteractiveCoder<AffMatrix>>(
      ldpca_dec, mp, syndromeMgr, llrProvider);
}

p_InteractiveCoder
CoderFactory::create_coder(const boost::program_options::variables_map &args) {
  return create_coder(parse_coder_parameters(args));
}

void CoderFactory::create_coder_yuv(const CoderParameters &args,
                                    p_InteractiveCoder &y_coder,
                                    p_InteractiveCoder &uv_coder) {

  // create parameters for Chroma coder: size of chroma block is 1/4 size of
  // luminance block
  auto args_uv(args);
  args_uv.n /= 4;
  // setCodeLength(args_uv, args[OPT_CODE_LENGTH].as<int>() / 4);

  y_coder = CoderFactory::create_coder(args);
  ICOV_INFO << "Luminance coder instanciated";

  uv_coder = CoderFactory::create_coder(args_uv);
  ICOV_INFO << "Chroma coder instanciated";
}

void CoderFactory::create_coder_yuv(
    const boost::program_options::variables_map &args,
    p_InteractiveCoder &y_coder, p_InteractiveCoder &uv_coder) {
  return create_coder_yuv(parse_coder_parameters(args), y_coder, uv_coder);
}

} // namespace icov
