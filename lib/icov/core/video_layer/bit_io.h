#pragma once

#include "icov_exception.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace icov {
namespace bitstream {

/// MSB-first bit writer with the u(n), ue(v), se(v) descriptors of ITU-T H.266
class BitWriter {
public:
  void u(unsigned n, uint64_t value) {
    for (unsigned i = n; i > 0; --i)
      bit((value >> (i - 1)) & 1);
  }

  void flag(bool value) { bit(value); }

  void ue(uint64_t value) {
    const uint64_t v = value + 1;
    unsigned len = 0;
    while ((v >> len) > 1)
      ++len;
    u(len, 0);
    u(len + 1, v);
  }

  void se(int64_t value) {
    ue(value > 0 ? 2 * uint64_t(value) - 1 : 2 * uint64_t(-value));
  }

  /// Appends the first nbits of an MSB-first byte array, without alignment
  void bits(const uint8_t *data, size_t nbits) {
    if (aligned()) {
      m_data.insert(m_data.end(), data, data + nbits / 8);
      m_bits += (nbits / 8) * 8;
    } else {
      for (size_t i = 0; i < (nbits / 8) * 8; ++i)
        bit((data[i / 8] >> (7 - i % 8)) & 1);
    }
    for (size_t i = (nbits / 8) * 8; i < nbits; ++i)
      bit((data[i / 8] >> (7 - i % 8)) & 1);
  }

  /// Zero bits up to the next byte boundary
  void byte_alignment() {
    while (!aligned())
      bit(0);
  }

  /// Stop bit then zero bits, as rbsp_trailing_bits()
  void trailing_bits() {
    bit(1);
    byte_alignment();
  }

  bool aligned() const { return m_bits % 8 == 0; }
  size_t bit_count() const { return m_bits; }
  const std::vector<uint8_t> &data() const { return m_data; }
  void clear() {
    m_data.clear();
    m_bits = 0;
  }

private:
  void bit(bool b) {
    if (m_bits % 8 == 0)
      m_data.push_back(0);
    if (b)
      m_data.back() |= uint8_t(0x80 >> (m_bits % 8));
    ++m_bits;
  }

  std::vector<uint8_t> m_data;
  size_t m_bits = 0;
};

/// Reader matching BitWriter; throws when reading past the end
class BitReader {
public:
  BitReader() = default;
  BitReader(const uint8_t *data, size_t size) { reset(data, size); }
  explicit BitReader(const std::vector<uint8_t> &data)
      : BitReader(data.data(), data.size()) {}

  void reset(const uint8_t *data, size_t size) {
    m_data = data;
    m_size = size;
    m_pos = 0;
  }

  uint64_t u(unsigned n) {
    uint64_t v = 0;
    for (unsigned i = 0; i < n; ++i)
      v = (v << 1) | bit();
    return v;
  }

  bool flag() { return bit(); }

  uint64_t ue() {
    unsigned len = 0;
    while (!bit()) {
      if (++len > 63)
        ICOV_THROW_OUT_OF_RANGE("Invalid ue(v) code");
    }
    return ((uint64_t(1) << len) | u(len)) - 1;
  }

  int64_t se() {
    const uint64_t k = ue();
    return (k & 1) ? int64_t((k + 1) / 2) : -int64_t(k / 2);
  }

  /// Reads nbits into an MSB-first byte array of ceil(nbits / 8) bytes
  void bits(uint8_t *out, size_t nbits) {
    for (size_t i = 0; i < (nbits + 7) / 8; ++i)
      out[i] = 0;
    for (size_t i = 0; i < nbits; ++i)
      if (bit())
        out[i / 8] |= uint8_t(0x80 >> (i % 8));
  }

  void skip(size_t nbits) {
    check(nbits);
    m_pos += nbits;
  }

  void byte_alignment() { m_pos = (m_pos + 7) / 8 * 8; }

  bool aligned() const { return m_pos % 8 == 0; }
  size_t position() const { return m_pos; }
  size_t bits_left() const { return m_size * 8 - m_pos; }

private:
  void check(size_t nbits) const {
    if (m_pos + nbits > m_size * 8)
      ICOV_THROW_OUT_OF_RANGE("Read past the end of the bitstream");
  }

  bool bit() {
    check(1);
    const bool b = (m_data[m_pos / 8] >> (7 - m_pos % 8)) & 1;
    ++m_pos;
    return b;
  }

  const uint8_t *m_data = nullptr;
  size_t m_size = 0;
  size_t m_pos = 0;
};

} // namespace bitstream
} // namespace icov
