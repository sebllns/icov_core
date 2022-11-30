#pragma once

#include "common.h"
#include "helpers.h"
#include "icov_program_options.h"
#include "ldpc_codec_layer/IInteractiveCoder.h"
#include "tools/DebugTools.h"
#include <boost/exception/diagnostic_information.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/program_options.hpp>
#include <boost/stacktrace.hpp>
#include <boost/system/error_code.hpp>

namespace po = boost::program_options;

////////////////////////////////////////////////////////////////
class IIcovProgram {
protected:
  po::options_description m_options_desc;
  po::variables_map m_options_map;

public:
  IIcovProgram() : m_options_desc("Program Usage", 1024, 512) {}

  virtual ~IIcovProgram() = default;

  void processCommandLine(int argc, char **argv) {
    po::store(po::parse_command_line(argc, argv, m_options_desc),
              m_options_map);
  }

  virtual void launchProgram() = 0;
  virtual int logLevel() const { return 0; }

  inline const boost::program_options::options_description &
  optionsDesc() const {
    return m_options_desc;
  }
};

////////////////////////////////////////////////////////////////
class IcovProgramBase : public IIcovProgram {
public:
  IcovProgramBase() {
    this->m_options_desc.add_options()

        ("help,h", "produce help message")(
            (std::string(OPT_MODE) + ",m").c_str(),
            po::value<std::string>()->required(), "set the program mode")(
            (std::string(OPT_CODER_TYPE) + ",c").c_str(),
            po::value<std::string>()->required(), "set the coder type")(
            (std::string(OPT_DATA_PATH) + ",d").c_str(),
            po::value<std::string>()->required(), "set the data path")(
            (std::string(OPT_CODE_LENGTH) + ",n").c_str(),
            po::value<int>()->required(),
            "set the code length")((std::string(OPT_BP_ITE) + ",it").c_str(),
                                   po::value<int>()->default_value(100),
                                   "set iteration count for BP LDPC coder")(
            OPT_NUMCODES,
            po::value<std::string>()->default_value("numcodes.txt"),
            "set the numcodes steps for iterative coder")(
            (std::string(OPT_LOG_LVL) + ",lg").c_str(),
            po::value<int>()->default_value(2), "set log level filter");
  }

  virtual ~IcovProgramBase() = default;

  const std::string &programMode() const {
    return this->m_options_map[OPT_MODE].as<std::string>();
  }

  const std::string &coderType() const {
    return this->m_options_map[OPT_CODER_TYPE].as<std::string>();
  }

  const std::string &dataPath() const {
    return this->m_options_map[OPT_DATA_PATH].as<std::string>();
  }

  int codeLength() const {
    return this->m_options_map[OPT_CODE_LENGTH].as<int>();
  }

  int logLevel() const override {
    return this->m_options_map[OPT_LOG_LVL].as<int>();
  }

  int bp_ite() const { return this->m_options_map[OPT_BP_ITE].as<int>(); }

  void launchProgram() {
    if (m_options_map.count(OPT_HELP)) {
      std::cout << m_options_desc << std::endl;
      return;
    } else {
      po::notify(m_options_map);
    }

    launchMode(programMode());
  }

  virtual void launchMode(const std::string &m) = 0;
};

////////////////////////////////////////////////////////////////
static int icov_main(int argc, char *argv[], IIcovProgram &pg) {
  icov::helpers::rand::init();

  icov::IInteractiveCoder::params.CheckBitplaneEncoding =
#ifdef ICOV_CHECK_BITPLANE_ENCODING_ENABLED
      true;
#else
      false;
#endif

  try {
    boost::log::add_common_attributes();
    DebugTools::initLog();
    pg.processCommandLine(argc, argv);
    boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                        pg.logLevel());
    pg.launchProgram();
  } catch (std::exception &e) {
    ICOV_LOG(info) << pg.optionsDesc() << std::endl;
    ICOV_LOG(fatal) << std::endl
                    << "Error: program execution has stopped prematurely"
                    << std::endl;
    ICOV_LOG(fatal) << boost::current_exception_diagnostic_information()
                    << std::endl;
#ifdef ICOV_CORE_STACKTRACE_ENABLED
    ICOV_LOG(fatal) << boost::stacktrace::stacktrace();
#endif
    return boost::system::errc::interrupted;
  } catch (...) {
    ICOV_LOG(info) << pg.optionsDesc() << std::endl;
    ICOV_LOG(fatal) << "Fatal error" << std::endl;
    ICOV_LOG(fatal) << std::endl
                    << boost::current_exception_diagnostic_information()
                    << std::endl;
#ifdef ICOV_CORE_STACKTRACE_ENABLED
    ICOV_LOG(fatal) << boost::stacktrace::stacktrace();
#endif
    return boost::system::errc::interrupted;
  }
  return boost::system::errc::success;
}
