#pragma once

#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "ldpc_codec_layer/factory/LdpcCoderParameters.h"
#include "ldpc_codec_layer/factory/aff3ct_coder_types.h"
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/variables_map.hpp>
#include <cstdint>
#include <exception>
#include <memory>
#include <queue>
#include <string>
#include <utility>

namespace po = boost::program_options;

namespace icov
{
namespace options {

typedef po::variables_map args_t;
typedef po::options_description desc_t;

const std::string code_length = "code-length";
const std::string icu_size = "icu-size";
const std::string bp_iter = "bp-ite";
const std::string data_path = "data-path";
const std::string coder_type = "coder-type";
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
const std::string csv_output = "csv-output";
const std::string positions_input = "positions-input";
const std::string dummy_viewport = "dummy-viewport";
const std::string headless = "headless";
const std::string ground_truth = "ground-truth";

inline void addGroundPathOption(desc_t &output)
{
    output.add_options()(ground_truth.c_str(), po::value<std::string>()->default_value(""),
                         "Path to use ground truth when non-empty");
}

inline void addHeadlessOption(desc_t &output)
{
    output.add_options()(headless.c_str(), po::value<bool>()->default_value(false),
                         "Use headless decoder which means no graphical output (only meta-parameters will be extracted "
                         "from encoded file) but is faster to execute. Will use ground truth when given, "
                         "to replace graphical output");
}

inline void addPreEncodeOption(desc_t &output)
{
    output.add_options()(pre_encode.c_str(), po::value<bool>()->default_value(false),
                         "Pre-encode the media before decode (useful to get more debug informations)");
}

inline void addCsvOutputOption(desc_t &output)
{
    output.add_options()(csv_output.c_str(), po::value<std::string>()->default_value(""),
                         "Prefix path if you need to export decoding results into CSV");
}

inline void addPositionsInputOption(desc_t &output)
{
    output.add_options()(positions_input.c_str(), po::value<std::string>()->default_value(""),
                         "Path pointing to file that contains viewpoint positions for decoding simulation");
}

inline void addViewportPaddingOption(desc_t &output)
{
    output.add_options()(viewport_padding.c_str(), po::value<float>()->default_value(1.2),
                         "Radius factor used in icov viewport to check if a block is "
                         "visible");
}

inline void addViewportFovOption(desc_t &output)
{
    output.add_options()((fov + ",f").c_str(), po::value<float>()->default_value(80),
                         "Viewport horizontal field of view in degrees");
}

inline void addDummyViewportOption(desc_t &output)
{
    output.add_options()(dummy_viewport.c_str(), po::value<bool>()->default_value(false),
                         "Use dummy viewport (all blocks are visible)");
}

inline void init_options_desc(const std::string &modes, desc_t &output)
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

                            ((log_lvl + ",lg").c_str(), po::value<int>()->default_value(2), "Set log level filter")

                                ((input_file + ",i").c_str(),
                                 po::value<std::string>()->default_value("/tmp/input_file"), "Set input file")

                                    ((output_file + ",o").c_str(), po::value<std::string>()->default_value(""),
                                     "Set output file");

    addPreEncodeOption(output);
    addCsvOutputOption(output);
    addPositionsInputOption(output);
    addViewportPaddingOption(output);
    addViewportFovOption(output);
    addDummyViewportOption(output);
    addHeadlessOption(output);
    addGroundPathOption(output);
}

#define ICOV_DATA_DIR(args) fs::absolute(fs::path(args[icov::options::data_path].as<std::string>()))
#define ICOV_Q_VALUES_FILE(args) (ICOV_DATA_DIR(args) / "../q-ary/q")

template <typename T> T parse(const std::string &id, const args_t &args)
{
    return args[id].as<T>();
}

inline std::string parseInputFile(const args_t &args)
{
    return args[input_file].as<std::string>();
}

inline std::string parseOutputFile(const args_t &args)
{
    return args[output_file].as<std::string>();
}

inline std::string parseCsvOutputFile(const args_t &args)
{
    return args[csv_output].as<std::string>();
}

inline std::string parsePositionsInputFile(const args_t &args)
{
    return args[positions_input].as<std::string>();
}

inline std::string parseLdpcCoderType(const args_t &args)
{
    return args[coder_type].as<std::string>();
}

inline std::string parseDatabase(const args_t &args)
{
    return args[data_path].as<std::string>();
}

inline int parseCodeLength(const args_t &args)
{
    return args[code_length].as<int>();
}

inline int parseBpIte(const args_t &args)
{
  return args[bp_iter].as<int>();
}

inline float parseFov(const args_t &args)
{
  return args[fov].as<float>();
}

inline float parseViewportPadding(const args_t &args)
{
  return args[viewport_padding].as<float>();
}

inline bool parsePreEncodeOption(const args_t &args)
{
  return args[pre_encode].as<bool>();
}

inline bool parseDummyViewportOption(const args_t &args)
{
  return args[dummy_viewport].as<bool>();
}

inline void parseLdpcParams(const args_t &args, const std::string &intented_type, LdpcCoderParameters &params)
{
  icov::assert::equal(params.type(), intented_type,
                      "Given type was \'" + params.type() + "\' but intended type was \'" + intented_type + "\'");

  params.database = parseDatabase(args);
  params.codeLength = parseCodeLength(args);
  params.beliefPropagationIterations = parseBpIte(args);
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

struct encoder_param_t
{
  uint16_t bp_iter;
  int8b qp;
  float viewport_padding;
  float fov;
  bool encode_allblocks;
  std::string input_media_file;
  std::string output_icov_file;
};

} // namespace options
} // namespace icov
