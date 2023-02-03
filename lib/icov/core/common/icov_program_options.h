#pragma once

#include <boost/program_options/variables_map.hpp>
#define OPT_CODE_LENGTH "code-length"
#define OPT_BP_ITE "bp-ite"
#define OPT_NUMCODES "numcodes"
#define OPT_DATA_PATH "data-path"
#define OPT_CODER_TYPE "coder"
#define OPT_HELP "help"
#define OPT_MODE "mode"
#define OPT_LOG_LVL "log-level"

#define ICOV_DATA_DIR                                                          \
  fs::absolute(fs::path(args[OPT_DATA_PATH].as<std::string>()))
#define ICOV_Q_VALUES_FILE (ICOV_DATA_DIR / "../q-ary/q")

inline int getCodeLength(const boost::program_options::variables_map &args) {
  return args[OPT_CODE_LENGTH].as<int>();
}

inline void setCodeLength(boost::program_options::variables_map &args,
                          int value) {
  boost::any uv_length(value);
  auto code_length_option = args[OPT_CODE_LENGTH];
  code_length_option.value() = uv_length;
  args.insert_or_assign(OPT_CODE_LENGTH, code_length_option);
}
