#pragma once

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
