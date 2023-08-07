#pragma once

#include "common.h"

#include "icov_program_options.h"
#include "ldpc_codec_layer/interface/IInteractiveCoder.h"

#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>
#include <boost/stacktrace.hpp>
#include <boost/system/error_code.hpp>
#include <string>

using namespace icov::options;
namespace po = boost::program_options;
typedef boost::error_info<struct tag_stacktrace, boost::stacktrace::stacktrace>
    traced;

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

  const po::variables_map &args() const
  {
    return m_options_map;
  }
};

////////////////////////////////////////////////////////////////
class IcovProgramBase : public IIcovProgram {
public:
  IcovProgramBase(const std::string &modes) {

    //    this->m_options_desc.add_options()

    //        ("help,h", "produce help message")

    //            ((std::string(mode) + ",m").c_str(),
    //             po::value<std::string>()->required(),
    //             (std::string("Set the program mode (see documentation for
    //             more "
    //                          "details on each mode)\nAccepted values: ") +
    //              modes)
    //                 .c_str())

    //                ((std::string(coder_type) + ",c").c_str(),
    //                 po::value<std::string>()->default_value("imt164"),
    //                 "Set the coder type")

    //                    ((std::string(data_path) + ",d").c_str(),
    //                     po::value<std::string>()->default_value("data/"),
    //                     "Set the data path")

    //                        ((std::string(code_length) + ",n").c_str(),
    //                         po::value<int>()->default_value(1024),
    //                         "set the code length")

    //                            ((std::string(bp_iter) + ",it").c_str(),
    //                             po::value<int>()->default_value(100),
    //                             "Set iteration count for belief propagation
    //                             of " "LDPC coder")

    //                                (numcodes.c_str(),
    //                                 po::value<std::string>()->default_value(
    //                                     "numcodes.txt"),
    //                                 "Set the numcodes steps for iterative
    //                                 coder")

    //                                    ((std::string(log_lvl) +
    //                                    ",lg").c_str(),
    //                                     po::value<int>()->default_value(2),
    //                                     "Set log level filter");

    init_options_desc(modes, m_options_desc);
  }

  virtual ~IcovProgramBase() = default;

  const std::string &programMode() const {
    return this->m_options_map[mode].as<std::string>();
  }

  const std::string &coderType() const {
    return this->m_options_map[coder_type].as<std::string>();
  }

  const std::string &dataPath() const {
    return this->m_options_map[data_path].as<std::string>();
  }

  int codeLength() const { return this->m_options_map[code_length].as<int>(); }

  int logLevel() const override {
    return this->m_options_map[log_lvl].as<int>();
  }

  int bp_ite() const { return this->m_options_map[bp_iter].as<int>(); }

  void launchProgram() override {
    if (m_options_map.count(help)) {
      ICOV_INFO << m_options_desc;
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
  icov::maths::rand::init();

  icov::ldpc_codec_layer::IInteractiveCoder::params.CheckBitplaneEncoding =
#ifdef ICOV_CHECK_BITPLANE_ENCODING_ENABLED
      true;
#else
      false;
#endif

  try {
    icov::logging::init_log(true);
    pg.processCommandLine(argc, argv);
    icov::logging::set_filter(pg.logLevel());
    pg.launchProgram();
  } catch (std::exception &e) {
    ICOV_INFO << pg.optionsDesc() << std::endl;
    ICOV_LOG(fatal) << std::endl
                    << "Error: program execution has stopped prematurely"
                    << std::endl;
    ICOV_LOG(fatal) << boost::current_exception_diagnostic_information()
                    << std::endl;
#ifdef ICOV_CORE_STACKTRACE_ENABLED
    const boost::stacktrace::stacktrace *st = boost::get_error_info<traced>(e);
    if (st) {
      ICOV_LOG(fatal) << *st;
    } else {
      ICOV_LOG(fatal) << boost::stacktrace::stacktrace();
    }
#endif
    return boost::system::errc::interrupted;
  } catch (...) {
    ICOV_INFO << pg.optionsDesc() << std::endl;
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
