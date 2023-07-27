#pragma once

#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "ldpc_codec_layer/factory/Coder_IMT64.h"
#include "ldpc_codec_layer/factory/LdpcCoderParameters.h"
#include "ldpc_codec_layer/factory/aff3ct_coder_types.h"
#include <boost/program_options.hpp>
#include <boost/program_options/variables_map.hpp>
#include <memory>

namespace po = boost::program_options;

//#define code_length "code-length"
//#define bp_ite "bp-ite"
//#define numcodes "numcodes"
//#define data_path "data-path"
//#define coder_type "coder"
//#define OPT_HELP "help"
//#define OPT_MODE "mode"
//#define OPT_LOG_LVL "log-level"

namespace icov {
namespace options {

const std::string code_length = "code-length";
const std::string bp_iter = "bp-ite";
const std::string numcodes = "numcodes";
const std::string data_path = "data-path";
const std::string coder_type = "coder";
const std::string help = "help";
const std::string mode = "mode";
const std::string log_lvl = "log-level";
const std::string input_file = "input-file";
const std::string output_file = "output-file";
const std::string qp = "qp";
const std::string viewport_padding = "viewport-padding";
const std::string encode_allblocks = "encode-allblocks";
const std::string start_block = "start-block";
const std::string block_count = "block-count";

inline void init_options_desc(const std::string &modes,
                              po::options_description &output) {

  //  output.add_options()

  //      ("help,h", "Produce help message")

  //          ((mode + ",m").c_str(), po::value<std::string>()->required(),
  //           (std::string("Set the program mode (see documentation for more "
  //                        "details on each mode)\nAccepted values: ") +
  //            modes)
  //               .c_str())

  //              ((coder_type + ",c").c_str(),
  //               po::value<std::string>()->default_value("imt164"),
  //               "Set the coder type")

  //                  ((data_path + ",d").c_str(),
  //                   po::value<std::string>()->default_value("data/"),
  //                   "Set the data path")

  //                      ((code_length + ",n").c_str(),
  //                       po::value<int>()->default_value(1024),
  //                       "Set the codeword length for LDPC")(
  //                          (std::string(bp_iter) + ",it").c_str(),
  //                          po::value<int>()->default_value(100),
  //                          "Set iteration count for belief propagation of
  //                          LDPC " "coder")

  //                          (numcodes.c_str(),
  //                           po::value<std::string>()->default_value(
  //                               "numcodes.txt"),
  //                           "Set the numcodes steps for iterative coder")

  //                              ((log_lvl + ",lg").c_str(),
  //                               po::value<int>()->default_value(2),
  //                               "Set log level filter");
}

#define ICOV_DATA_DIR(args)                                                    \
  fs::absolute(fs::path(args[icov::options::data_path].as<std::string>()))
#define ICOV_Q_VALUES_FILE(args) (ICOV_DATA_DIR(args) / "../q-ary/q")

inline std::string parseType(const po::variables_map &args) {
  return args[coder_type].as<std::string>();
}

inline std::string parseDatabase(const po::variables_map &args) {
  return args[data_path].as<std::string>();
}

inline int parseCodeLength(const po::variables_map &args) {
  return args[code_length].as<int>();
}

inline int parseBpIte(const po::variables_map &args) {
  return args[bp_iter].as<int>();
}

inline std::string parseNumcodeFile(const po::variables_map &args) {
  return args[numcodes].as<std::string>();
}

inline void parseLdpcParams(const po::variables_map &args,
                            const std::string &intented_type,
                            LdpcCoderParameters &params) {

  params.type = parseType(args);
  params.database = parseDatabase(args);
  params.codeLength = parseCodeLength(args);
  params.beliefPropagationIterations_new = parseBpIte(args);
  params.numcodeFile = parseNumcodeFile(args);

  icov::assert::equal(params.type, intented_type,
                      "Given type was \'" + params.type +
                          "\' but intended type was \'" + intented_type + "\'");
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

  icov::assert::not_null(params.get());
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
