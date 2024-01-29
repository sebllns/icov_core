#pragma once

#include "icov_log.h"
#include "icov_program_options.h"
#include "ldpc_codec_layer/interface/IInteractiveCoder.h"
#include "video_layer/video_common.h"

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
  desc_t m_options_desc;
  args_t m_options_map;

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

  const args_t &args() const
  {
    return m_options_map;
  }
};

////////////////////////////////////////////////////////////////
class IcovProgramBase : public IIcovProgram {
public:
  IcovProgramBase(const std::string &modes) {
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
inline int icov_main(int argc, char *argv[], IIcovProgram &pg)
{
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
    icov::video::init_video_lib(argc, argv);
    pg.launchProgram();
  } catch (std::exception &e) {
    ICOV_INFO << std::endl << pg.optionsDesc() << std::endl;
    ICOV_FATAL << std::endl << "Error: program execution has stopped prematurely" << std::endl;
    ICOV_FATAL << boost::current_exception_diagnostic_information() << std::endl;
#ifdef ICOV_CORE_STACKTRACE_ENABLED
    const boost::stacktrace::stacktrace *st = boost::get_error_info<traced>(e);
    if (st) {
      ICOV_FATAL << *st;
    } else {
      ICOV_FATAL << boost::stacktrace::stacktrace() << std::endl;
    }
#endif
    return boost::system::errc::interrupted;
  } catch (...) {
    ICOV_INFO << std::endl << pg.optionsDesc() << std::endl;
    ICOV_FATAL << "Fatal error" << std::endl;
    ICOV_FATAL << std::endl << boost::current_exception_diagnostic_information() << std::endl;
#ifdef ICOV_CORE_STACKTRACE_ENABLED
    ICOV_FATAL << boost::stacktrace::stacktrace() << std::endl;
#endif
    return boost::system::errc::interrupted;
  }
  return boost::system::errc::success;
}
