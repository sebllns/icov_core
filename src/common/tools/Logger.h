/****************************************************************************/
/** @addtogroup icov-core 
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup icov-core
 */

/** @file Logger.h
 *  @brief Class used to manage log.
 *  @ingroup common
 *  @author Sébastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 0.0.1
 *  @date 2021
 *  @copyright Copyright (C) 2021 by Inria. All rights reserved.
 * The copyright in this software is being made available under 
 * the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version. 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU Affero General Public License for more details.
 * <br/>
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * <br/>
 * <b> For using ICOV under terms that will not comply with the GNU
 * AGPL, please contact Inria about acquiring an ICOV Professional
 * Edition License. </b>
 * <br/>
 * See contact section in README file for more information.
 * <br/>
 * This software was developed at:
 * Inria Rennes - Bretagne Atlantique
 * Campus Universitaire de Beaulieu
 * 35042 Rennes Cedex
 * France
 * <br/>
 * If you have questions regarding the use of this software, please contact
 * Inria using the contact section in the README file.
 * <br/>
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *****************************************************************************/

#ifndef CONTENTPARAMETERSET_H
#define CONTENTPARAMETERSET_H

#include "common.h"
#include <sstream>
#include <stdio.h>

#define log_condition(c) ((c) ? "true" : "false")

//#if WIN
//#define DLLExport __declspec(dllexport)

using namespace std;

namespace icov
{
  extern "C"
  {
    //Create a callback delegate
    typedef void (*LogCallBack)(const char *message, int length);
  }

  class Logger : public std::streambuf
  {
  public:
    enum_t::LogType logLevel;
    enum_t::LogType logLevelFilter;

    bool showTimestamp;
    bool showLogLevel;
    bool showLogConsole;

    Logger(std::basic_ios<char> &out, bool showLogLevel_ = true, bool addTimestamp_ = false, enum_t::LogType logLevelFilter_ = enum_t::LogType::LOG_TYPE_INFO, bool showLogConsole_ = true)
        : m_out(out), m_sink(), m_newline(true), m_callback(nullptr),
          showLogLevel(showLogLevel_),
          showTimestamp(addTimestamp_),
          showLogConsole(showLogConsole_),
          logLevel(enum_t::LogType::LOG_TYPE_INFO), logLevelFilter(logLevelFilter_)
    {
      setp(0, 0);
      m_sink = m_out.rdbuf(this);
      m_pstream = new std::ostream(m_sink);
      assert(m_sink);
    }
    ~Logger()
    {
      if (m_logfile.is_open())
        m_logfile.close();

      m_out.rdbuf(m_sink);
    }

    std::streambuf *getSink()
    {
      return m_sink;
    }

    // Get current date/time, format is YYYY-MM-DD.HH:mm:ss
    static const std::string getTime(const char *format = "%Y-%m-%d.%X")
    {
      time_t now = time(0);
      struct tm tstruct;
      char buf[80];
      tstruct = *localtime(&now);
      // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
      // for more information about date/time format
      strftime(buf, sizeof(buf), format, &tstruct);

      return buf;
    }

    void logToFile(const char *filename)
    {
      m_logfile.open(filename, ios::out);
    }

    void redirect(LogCallBack callback)
    {
      m_callback = callback;
      if (m_pstream != nullptr)
      {
        delete m_pstream;
      }
      m_pstream = &m_stringstream;
    }

    bool callbackEnabled()
    {
      return m_callback != nullptr;
    }

    std::string LogTypeStr(enum_t::LogType LogType)
    {
      switch (LogType)
      {
      case enum_t::LogType::LOG_TYPE_INFO:
        return "INFO";
      case enum_t::LogType::LOG_TYPE_DEBUG:
        return "DEBUG";
      case enum_t::LogType::LOG_TYPE_WARNING:
        return "WARNING";
      case enum_t::LogType::LOG_TYPE_ERROR:
        return "ERROR";
      default:
        return "";
      }

      return "";
    }

  protected:
    bool writeHeader(std::ostream &str)
    {
      bool result = true;

      // --   add timestamp level here
      if (showTimestamp && !(str << "[" << getTime() << "] "))
        result = false;

      // --   add log level here
      if (showLogLevel && logLevel > 0 && logLevel < 6 && !(str << "[" << LogTypeStr(logLevel) << "] "))
        result = false;

      return result;
    }

    int_type overflow(int_type m = traits_type::eof())
    {
      if (traits_type::eq_int_type(m, traits_type::eof()))
        return m_sink->pubsync() == -1 ? m : traits_type::not_eof(m);

      if (logLevel < logLevelFilter)
        return traits_type::eof();

      if (m_newline && (showLogConsole || callbackEnabled()))
      {
        std::ostream &str = *m_pstream;

        if (!writeHeader(str))
        {
          traits_type::eof(); // Error
        }
      }

      if (m_logfile.is_open())
      {
        if (m_newline)
          writeHeader(m_logfile);
        m_logfile.put(traits_type::to_char_type(m));
      }

      m_newline = traits_type::to_char_type(m) == '\n';

      if (callbackEnabled())
      {

        m_stringstream.put(traits_type::to_char_type(m));

        if (m_newline)
        {
          const std::string tmp = m_stringstream.str();
          const char *tmsg = tmp.c_str();
          m_callback(tmsg, (int)strlen(tmsg));
          m_stringstream.str("");
          m_stringstream.clear();
        }

        return m;
      }

      if (showLogConsole)
        return m_sink->sputc(m);

      return m;
    }

  private:
    Logger(const Logger &);
    Logger &operator=(const Logger &); // not copyable

    std::basic_ios<char> &m_out;
    std::streambuf *m_sink;
    bool m_newline;
    std::stringstream m_stringstream;
    std::ostream *m_pstream;
    LogCallBack m_callback;
    ofstream m_logfile;
  };
}
#endif // CONTENTPARAMETERSET_H
