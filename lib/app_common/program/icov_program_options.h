#pragma once

#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "ldpc_codec_layer/factory/Coder_IMT64.h"
#include "ldpc_codec_layer/factory/LdpcCoderParameters.h"
#include "ldpc_codec_layer/factory/aff3ct_coder_types.h"
#include <boost/program_options.hpp>
#include <boost/program_options/variables_map.hpp>
#include <memory>

namespace po = boost::program_options;

namespace icov
{
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
const std::string fov = "fov";
const std::string encode_allblocks = "encode-allblocks";
const std::string start_block = "start-block";
const std::string block_count = "block-count";
const std::string pre_encode = "pre-encode";
const std::string viewport_type = "viewport-type";

inline void addPreEncodeOption(po::options_description &output)
{
    output.add_options()(pre_encode.c_str(), po::value<bool>()->default_value(false),
                         "Pre-encode the media before decode (useful to get more debug informations)");
}

inline void init_options_desc(const std::string &modes, po::options_description &output)
{
    output.add_options()

        ("help,h", "Produce help message")

            ((mode + ",m").c_str(), po::value<std::string>()->required(),
             (std::string("Set the program mode (see documentation for more "
                          "details on each mode)\nAccepted values: ") +
              modes)
                 .c_str())

                ((coder_type + ",c").c_str(), po::value<std::string>()->default_value("imt164"), "Set the coder type")

                    ((data_path + ",d").c_str(), po::value<std::string>()->default_value("data/"), "Set the data path")

                        ((code_length + ",n").c_str(), po::value<int>()->default_value(1024),
                         "Set the codeword length for LDPC")((std::string(bp_iter) + ",it").c_str(),
                                                             po::value<int>()->default_value(100),
                                                             "Set iteration count for belief propagation of LDPC "
                                                             "coder")

                            (numcodes.c_str(), po::value<std::string>()->default_value("numcodes.txt"),
                             "Set the numcodes steps for iterative coder")

                                ((log_lvl + ",lg").c_str(), po::value<int>()->default_value(2), "Set log level filter")

                                    ((input_file + ",i").c_str(),
                                     po::value<std::string>()->default_value("/tmp/input_file"), "Set input file")

                                        ((output_file + ",o").c_str(), po::value<std::string>()->default_value(""),
                                         "Set output file")

                                            ((fov + ",f").c_str(), po::value<float>()->default_value(80),
                                             "Viewport horizontal field of view in degrees")

                                                (viewport_padding.c_str(), po::value<float>()->default_value(1.2),
                                                 "Radius factor used in icov viewport to check if a block is "
                                                 "visible");

    addPreEncodeOption(output);
}

#define ICOV_DATA_DIR(args) fs::absolute(fs::path(args[icov::options::data_path].as<std::string>()))
#define ICOV_Q_VALUES_FILE(args) (ICOV_DATA_DIR(args) / "../q-ary/q")

template <typename T> T parse(const std::string &id, const po::variables_map &args)
{
    return args[id].as<T>();
}

inline std::string parseInputFile(const po::variables_map &args)
{
    return args[input_file].as<std::string>();
}

inline std::string parseOutputFile(const po::variables_map &args)
{
    return args[output_file].as<std::string>();
}

inline std::string parseLdpcCoderType(const po::variables_map &args)
{
    return args[coder_type].as<std::string>();
}

inline std::string parseDatabase(const po::variables_map &args)
{
    return args[data_path].as<std::string>();
}

inline int parseCodeLength(const po::variables_map &args) {
  return args[code_length].as<int>();
}

inline int parseBpIte(const po::variables_map &args)
{
  return args[bp_iter].as<int>();
}

inline std::string parseNumcodeFile(const po::variables_map &args)
{
  return args[numcodes].as<std::string>();
}

inline float parseFov(const po::variables_map &args)
{
  return args[fov].as<float>();
}

inline float parseViewportPadding(const po::variables_map &args)
{
  return args[viewport_padding].as<float>();
}

inline void parseLdpcParams(const po::variables_map &args, const std::string &intented_type,
                            LdpcCoderParameters &params)
{

  params.type = parseLdpcCoderType(args);
  params.database = parseDatabase(args);
  params.codeLength = parseCodeLength(args);
  params.beliefPropagationIterations_new = parseBpIte(args);
  params.numcodeFile = parseNumcodeFile(args);

  icov::assert::equal(params.type, intented_type,
                      "Given type was \'" + params.type +
                          "\' but intended type was \'" + intented_type + "\'");
}

inline std::unique_ptr<LdpcCoderParameters> parseLdpcParams(const boost::program_options::variables_map &args)
{
  std::string coder_type_val = parseLdpcCoderType(args);
  std::unique_ptr<LdpcCoderParameters> params = ldpc_codec_layer::createLdpcParams(coder_type_val);
  icov::assert::not_null(params.get());
  parseLdpcParams(args, coder_type_val, *params);
  return params;
}

inline ldpc_codec_layer::p_InteractiveCoder create_coder(const boost::program_options::variables_map &args)
{
  auto params = parseLdpcParams(args);
  return ldpc_codec_layer::LdpcCoderFactory::create_coder(*params);
}
} // namespace options
} // namespace icov
