#pragma once

#include <boost/stacktrace/stacktrace.hpp>
#include <cstddef>

#include "typedef.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits.h>
#include <numeric>
#include <sstream>
#include <vector>
namespace icov {

///
///
/// @fixme seg fault when using the constructor
/// std::vector<FastArray<T>>(m, FastArray<T>(n))
template <typename T> class FastArray {
public:
  FastArray() : m_capacity(0), m_size(0), m_data(nullptr) {
    //    std::cout << "new empty " << m_size << std::endl;
  }

  FastArray(const FastArray &f) : m_capacity(f.m_capacity), m_size(f.m_size) {
    //    std::cout << "new copy " << m_size << std::endl;
    // std::cout << boost::stacktrace::stacktrace();
    //    ICOV_ASSERT_MSG(m_capacity >= m_size,
    //                    "Capacity is greater or equal than size");
    if (m_capacity > 0)
      m_data = new T[f.m_capacity];
    if (m_size > 0)
      std::memcpy(m_data, f.m_data, m_size * sizeof(T));
  }

  explicit FastArray(size_t size)
      : m_capacity(size), m_size(size), m_data(new T[size]) {}

  FastArray(T *data, size_t size)
      : m_capacity(size), m_size(size), m_data(data) {}

  virtual ~FastArray() {
    if (m_capacity > 0 && m_data != nullptr) {
      delete[] m_data;
      m_data = nullptr;
    }
  }

  FastArray<T> &operator=(const FastArray<T> &f) {

    if (m_capacity > 0) {
      if (m_data != nullptr)
        delete[] m_data;
      m_data = nullptr;
    }

    this->m_capacity = f.m_capacity;
    this->m_size = f.m_size;

    if (m_capacity > 0) {
      m_data = new T[m_capacity];
      if (m_size > 0)
        std::memcpy(m_data, f.m_data, m_size * sizeof(T));
    }

    //    std::cout << "copy " << m_capacity << " " << m_size << std::endl;

    return *this;
  }

  T operator[](int i) const { return m_data[i]; }
  T &operator[](int i) { return m_data[i]; }

  T at(int i) const { return m_data[i]; }
  T &at(int i) { return m_data[i]; }

  void fill(T val) { std::fill(m_data, m_data + size(), val); }

  size_t size() const { return m_size; }
  size_t capacity() const { return m_capacity; }

  void setData(T *data) { m_data = data; };
  const T *data() const { return m_data; }
  std::vector<T> getDataVec() const {
    return std::vector<T>(m_data, m_data + m_size);
  }

  template <typename T1> void fillFrom(const std::vector<T1> &v) {
    resize(v.size());
    for (size_t i = 0; i < m_size; ++i) {
      this->at(i) = static_cast<T>(v[i]);
    }
  }

  template <typename T1> std::vector<T1> getDataVec() const {
    return std::vector<T1>(m_data, m_data + m_size);
  }

  void resize(size_t new_size) {
    if (new_size > m_capacity) {
      T *tmp = m_data;
      m_data = new T[new_size];

      if (tmp != nullptr && m_capacity > 0 && m_size > 0) {
        std::memcpy(m_data, tmp, m_size * sizeof(T));
        delete[] tmp;
      }
      m_capacity = new_size;
    }
    m_size = new_size;
  }

  std::string to_string(const std::string &sep = ", ") const {
    std::stringstream sstr;

    sstr << "Array " << typeid(T).name() << m_size << "; max" << m_capacity
         << std::endl;

    if (m_size > 0) {

      sstr << "[ ";

      for (int k = 0; k < m_size - 1; k++) {

        sstr << this->m_data[k] << sep;
      }

      sstr << this->m_data[m_size - 1] << " ]" << std::endl;
    }
    return sstr.str();
  }

protected:
  T *m_data;
  size_t m_capacity;
  size_t m_size;
};

template <typename T> class FastMatrix : public FastArray<T> {
public:
  FastMatrix() : FastArray<T>(0), m_rows(0), m_cols(0) {}

  FastMatrix(size_t rows, size_t cols)
      : FastArray<T>(rows * cols), m_rows(rows), m_cols(cols) {
    // std::cout << "new " << this->m_size << std::endl;
  }

  FastMatrix(const FastMatrix &f)
      : FastArray<T>(f.m_rows * f.m_cols), m_rows(f.m_rows), m_cols(f.m_cols) {
    //    std::cout << "new copy " << this->m_size << std::endl;
    std::memcpy(this->m_data, f.m_data, this->m_size * sizeof(T));
  }

  FastMatrix<T> &operator=(const FastMatrix<T> &f) {
    this->m_rows = f.m_rows;
    this->m_cols = f.m_cols;

    if (this->m_data != nullptr) {
      delete[] this->m_data;
      this->m_data = nullptr;
    }
    this->m_capacity = f.m_capacity;
    this->m_size = f.m_size;
    this->m_data = new T[this->m_capacity];
    std::memcpy(this->m_data, f.m_data, this->m_size * sizeof(T));

    // std::cout << "copy " << this->m_capacity << " " << this->m_size
    //        << std::endl;

    return *this;
  }

  T at(int i, int j) const { return this->m_data[i * m_cols + j]; }

  T &at(int i, int j) {
    // std::cout << i*m_cols + j << std::endl;
    return this->m_data[i * m_cols + j];
  }

  std::string to_string() const {
    std::stringstream sstr;

    sstr << "Matrix " << typeid(T).name() << m_rows << "x" << m_cols
         << std::endl;

    for (int l = 0; l < m_cols; l++) {
      sstr << "\t[" << l << "]";
    }

    sstr << std::endl;
    sstr << std::endl;

    for (int k = 0; k < m_rows; k++) {

      sstr << "[" << k << "]\t";

      for (int l = 0; l < m_cols; l++) {
        sstr << this->m_data[k * m_cols + l] << '\t';
      }

      sstr << std::endl;
    }

    return sstr.str();
  }

  size_t rows() const { return m_rows; }
  size_t cols() const { return m_cols; }

  void resize(size_t new_size) = delete;

  void resize(size_t rows, size_t cols) {
    m_rows = rows;
    m_cols = cols;
    this->resize(rows * cols);
  }

protected:
  size_t m_rows;
  size_t m_cols;
};

typedef bool bit;
typedef float LLRVal;
typedef int SymboleVal;
typedef int BitVal;
typedef int SIVal;
typedef bit SyndVal;
typedef uint16_t index_t;

// FastArray is faster
using Syndrome = FastArray<bit>;
using BitVec = std::vector<BitVal>;
using SymboleVec = std::vector<SymboleVal>;
using LLRVec = std::vector<LLRVal>;
using SIVec = std::vector<SIVal>;
using Bitplanes = std::vector<BitVec>;
using Rate = float;
using NumcodeIdx = uint32_t;

// Bitplane
using SyndromesVec = std::vector<Syndrome>;
using NumcodeIdxVec = std::vector<NumcodeIdx>;

using SINumcodeIdxVec = std::vector<NumcodeIdxVec>;

template <typename T>
std::vector<size_t> sort_indexes(const std::vector<T> &v,
                                 std::vector<size_t> &idx) {
  // initialize original index locations
  // std::vector<size_t> idx(v.size());
  // std::iota(idx.begin(), idx.end(), 0);

  // sort indexes based on comparing values in v
  // using std::stable_sort instead of std::sort
  // to avoid unnecessary index re-orderings
  // when v contains elements of equal values
  std::stable_sort(idx.begin(), idx.end(),
                   [&v](size_t i1, size_t i2) { return v[i1] < v[i2]; });

  return idx;
}

template <typename T>
std::vector<size_t> sort_indexes_abs(const std::vector<T> &v,
                                     std::vector<size_t> &idx) {
  // initialize original index locations
  // std::vector<size_t> idx(v.size());
  // std::iota(idx.begin(), idx.end(), 0);

  // sort indexes based on comparing values in v
  // using std::stable_sort instead of std::sort
  // to avoid unnecessary index re-orderings
  // when v contains elements of equal values
  std::stable_sort(idx.begin(), idx.end(), [&v](size_t i1, size_t i2) {
    return std::fabs(v[i1]) < std::fabs(v[i2]);
  });

  return idx;
}

template <typename Iterator> struct value_type {
  typedef decltype(*(Iterator())) result;
};

} // namespace icov
