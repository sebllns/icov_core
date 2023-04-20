#pragma once

#include "helpers.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "ldpc_codec_layer/factory/Coder_IMT64.h"
#include "ldpc_codec_layer/factory/LdpcCoderParameters.h"
#include "ldpc_codec_layer/factory/aff3ct_coder_types.h"
#include <boost/program_options.hpp>
#include <boost/program_options/variables_map.hpp>
#include <memory>

namespace po = boost::program_options;

#define OPT_CODE_LENGTH "code-length"
#define OPT_BP_ITE "bp-ite"
#define OPT_NUMCODES "numcodes"
#define OPT_DATA_PATH "data-path"
#define OPT_CODER_TYPE "coder"
#define OPT_HELP "help"
#define OPT_MODE "mode"
#define OPT_LOG_LVL "log-level"

#define ICOV_DATA_DIR(args)                                                    \
  fs::absolute(fs::path(args[OPT_DATA_PATH].as<std::string>()))
#define ICOV_Q_VALUES_FILE(args) (ICOV_DATA_DIR(args) / "../q-ary/q")

namespace icov {
namespace options {

inline std::string parseType(const po::variables_map &args) {
  return args[OPT_CODER_TYPE].as<std::string>();
}

inline std::string parseDatabase(const po::variables_map &args) {
  return args[OPT_DATA_PATH].as<std::string>();
}

inline int parseCodeLength(const po::variables_map &args) {
  return args[OPT_CODE_LENGTH].as<int>();
}

inline int parseBpIte(const po::variables_map &args) {
  return args[OPT_BP_ITE].as<int>();
}

inline std::string parseNumcodeFile(const po::variables_map &args) {
  return args[OPT_NUMCODES].as<std::string>();
}

inline void parseLdpcParams(const po::variables_map &args,
                            const std::string &intented_type,
                            LdpcCoderParameters &params) {

  params.type = parseType(args);
  params.database = parseDatabase(args);
  params.codeLength = parseCodeLength(args);
  params.beliefPropagationInterations = parseBpIte(args);
  params.numcodeFile = parseNumcodeFile(args);

  icov::helpers::assert::equal(params.type, intented_type,
                               "Given type was \'" + params.type +
                                   "\' but intended type was \'" +
                                   intented_type + "\'");
}

inline std::unique_ptr<LdpcCoderParameters>
parseLdpcParams(const boost::program_options::variables_map &args) {
  std::string coder_type = parseType(args);
  std::unique_ptr<LdpcCoderParameters> params;

  if (coder_type == ldpc_codec_layer::LdpcCoder_IMT64::id) {
    params = std::make_unique<ldpc_codec_layer::LdpcCoder_IMT64::parameters>();
  } else {
    throw std::invalid_argument("Unknown coder type: " + coder_type);
  }

  icov::helpers::assert::not_null(params.get());
  parseLdpcParams(args, coder_type, *params);
  return params;
}

inline ldpc_codec_layer::p_InteractiveCoder
create_coder(const boost::program_options::variables_map &args) {
  auto params = parseLdpcParams(args);
  return ldpc_codec_layer::LdpcCoderFactory::create_coder(*params);
}
} // namespace options
} // namespace icov
