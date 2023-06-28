#pragma once

#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
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
  std::cout << ">>>>>>> READ POS " << str.tellg() << " SIZE " << size;
  str.read(reinterpret_cast<char *>(val), size * sizeof(T));
  std::cout << " VALUE = " << std::to_string(*val) << std::endl;
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
  std::ifstream file(filename);

  if (!file.is_open()) {
    throw std::invalid_argument("cannot open file " + filename);
  }

  int n = 0;
  file >> n;
  data = std::vector<T>(n);

  T val;
  for (int i = 0; i < data.size(); i++) {
    file >> val;
    data[i] = val;
  }
  file.close();
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

} // namespace io
} // namespace icov
