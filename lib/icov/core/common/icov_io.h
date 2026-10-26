#pragma once

#include "icov_assert.h"
#include "icov_data.h"
#include "icov_exception.h"
#include "icov_log.h"
#include "icov_string.h"
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#define WRITE_CHARS(buf, data) buf.write((char *)&data, sizeof(data))

namespace icov {
namespace io {

template <typename T>
std::ostream &write_vector(std::ostream &out, const std::vector<T> &data,
                           char sep) {
  out << data.size();
  for (int i = 0; i < data.size(); i++) {
    out << sep << data[i];
  }
  return out;
}

template <typename T>
static void write_array(std::ostream &str, const T *val, size_t size,
                        bool flush = false) {

  str.write(reinterpret_cast<const char *>(val), size * sizeof(T));
  if (flush) {
    str.flush();
  }
}

template <typename T>
static void write_value(std::ostream &str, const T &val, bool flush = false) {
  str.write(reinterpret_cast<const char *>(&val), sizeof(val));
  if (flush) {
    str.flush();
  }
}

template <typename T>
static void read_value(std::istream &str, T *val, int size = 1) {

  if (str.tellg() < 0) {
    ICOV_THROW_RUNTIME_EXCEPTION("Cannot read stream");
  }

  ICOV_TRACE << ">>>>>>> READ POS " << str.tellg() << " SIZE "
             << size * sizeof(T);
  str.read(reinterpret_cast<char *>(val), size * sizeof(T));
  ICOV_TRACE << " VALUE = " << std::to_string(*val);
}

template <typename T>
void write_vector(const std::string &filename, const std::vector<T> &data,
                  char sep) {
  std::ofstream file(filename);
  write_vector(file, data, sep);
  file.close();
}

template <typename T>
void read_vector(const std::string &filename, std::vector<T> &data) {
  auto in = icov::data::open(filename);
  std::istream &file = *in;

  int n = 0;
  file >> n;
  data = std::vector<T>(n);

  T val;
  for (int i = 0; i < data.size(); i++) {
    file >> val;
    data[i] = val;
  }
}

template <typename TInputIter>
std::string make_hex_string(TInputIter first, TInputIter last,
                            bool use_uppercase = true,
                            bool insert_spaces = false) {
  std::ostringstream ss;
  ss << std::hex << std::setfill('0');
  if (use_uppercase)
    ss << std::uppercase;
  while (first != last) {
    ss << std::setw(2) << static_cast<int>(*first++);
    if (insert_spaces && first != last)
      ss << " ";
  }
  return ss.str();
}

class CsvFileWriter
{
public:
  explicit CsvFileWriter(const std::string &path) : m_columnSeparator(","), m_file(path)
  {
  }

  void writeColumnNames()
  {
    std::stringstream ss = icov::string::join(m_columnsName, m_columnSeparator);
    m_file << ss.str() << std::endl;
  }

  void writeValues()
  {
    std::stringstream ss = icov::string::join(m_columnsValue, m_columnSeparator);
    m_file << ss.str() << std::endl;
  }

  void clear()
  {
    m_columnsName.clear();
    m_columnsValue.clear();
    m_columnsIdx.clear();
  }

  void clearValues()
  {
    m_columnsValue = std::vector<std::string>(m_columnsName.size());
  }

  void writeAndClearValues()
  {
    writeValues();
    clearValues();
  }

  void insertValue(const std::string &val)
  {
    m_columnsValue.push_back(val);
  }

  void addColumn(const std::string &name)
  {
    m_columnsIdx.insert(std::pair(name, m_columnsName.size()));
    m_columnsName.push_back(name);
    insertValue(std::string());
  }

  int getColumnCount() const
  {
    return m_columnsName.size();
  }

  int getColumnIdx(const std::string &name) const
  {
    return m_columnsIdx.at(name);
  }

  const std::string &getColumnName(int idx) const
  {
    return m_columnsName[idx];
  }

  const std::string &getColumnName(const std::string &name) const
  {
    return m_columnsName[getColumnIdx(name)];
  }

  const std::string &getColumnValue(int idx) const
  {
    return m_columnsValue[idx];
  }

  const std::string &getColumnValue(const std::string &name) const
  {
    return m_columnsValue[getColumnIdx(name)];
  }

  std::string &columnValue(int idx)
  {
    return m_columnsValue[idx];
  }

  std::string &columnValue(const std::string &name)
  {
    return m_columnsValue[getColumnIdx(name)];
  }

  template <typename T> void setColumnValue(int idx, const T &val)
  {
    columnValue(idx) = std::to_string(val);
  }

  template <typename T> void setColumnValue(const std::string &name, const T &val)
  {
    columnValue(name) = std::to_string(val);
  }

  void setColumnValue(int idx, const std::string &val)
  {
    columnValue(idx) = val;
  }

  void setColumnValue(const std::string &name, const std::string &val)
  {
    columnValue(name) = val;
  }

  const std::string &columnSeparator() const
  {
    return m_columnSeparator;
  }
  void setColumnSeparator(const std::string &newColumnSeparator)
  {
    m_columnSeparator = newColumnSeparator;
  }

private:
  std::vector<std::string> m_columnsName;
  std::vector<std::string> m_columnsValue;
  std::map<std::string, unsigned short> m_columnsIdx;
  std::string m_columnSeparator;
  std::ofstream m_file;
};

} // namespace io
} // namespace icov
