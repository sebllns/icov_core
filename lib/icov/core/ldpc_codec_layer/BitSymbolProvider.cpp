#include "BitSymbolProvider.h"

#include "helpers.h"
#include "tools/DebugTools.h"

namespace icov {
Bitplanes BitSymbolProvider::compute_bits(const SymboleVec &src_symbols) {
  int x_min = *std::min_element(src_symbols.begin(), src_symbols.end());
  int x_max = *std::max_element(src_symbols.begin(), src_symbols.end());
  int n = src_symbols.size();

  // get length for binary representation of X
  //    lx =  std::floor(std::log2f(std::max(std::abs(x_min), std::abs(x_max))))
  //    + 2;
  int lx = helpers::maths::binary_length(
               std::max(std::abs(x_min), std::abs(x_max))) +
           1;

  // init bit planes of X
  auto src_bits = Bitplanes(lx, BitVec(n, 0));

  for (int i = 0; i < n; i++) {
    int x_reconstruction = 0;
    src_bits[0][i] = (src_symbols[i] < 0);

    for (int j = 1; j < lx; j++) {
      src_bits[lx - j][i] = (std::abs(src_symbols[i]) >> (j - 1)) & 1U;
      x_reconstruction |= src_bits[lx - j][i] << (j - 1);
    }

    // sanity check for X binarization
    x_reconstruction *= (1 - 2 * src_bits[0][i]);
    if (x_reconstruction != src_symbols[i])
      throw std::runtime_error(
          "ERROR: Binarization failed: " + std::to_string(x_reconstruction) +
          " != " + std::to_string(src_symbols[i]));
  }

  return src_bits;
}

SymboleVec BitSymbolProvider::compute_symboles(const Bitplanes &src_bits) {
  int n = src_bits[0].size();
  int lx = src_bits.size();

  auto symboles = BitVec(n, 0);

  for (int i = 0; i < n; i++) {
    BitVal x_reconstruction = 0;
    for (int j = 1; j < lx; j++) {
      x_reconstruction |= src_bits[lx - j][i] << (j - 1);
    }

    symboles[i] = x_reconstruction * (1 - 2 * src_bits[0][i]);
  }

  return symboles;
}

const BitSymbolProvider BitSymbolProvider::init_bsp(
    const SymboleVec &src_symbols, const SIVec &side_info, int lx,
    bool enable_z_quantization, const std::vector<float> &q_values) {

  ICOV_ASSERT_MSG(src_symbols.size() == side_info.size(),
                  "Src and SI lengths are not equal")

  bool enable_q_quantization = q_values.size() > 0;
  size_t n = src_symbols.size();

  // init intrinsic SI parameters
  float q = 0.0;
  int z_min = 999999;
  int z_max = -999999;

  for (size_t i = 0; i < n; i++) {
    int z = src_symbols[i] - side_info[i];

    // q = N0/N
    if (z == 0) {
      q += 1.0 / n;
    }

    // z_min = min(Z[k], 0<=k<N)
    if (z < z_min)
      z_min = z;

    // z_max = max(Z[k], 0<=k<N)
    if (z > z_max)
      z_max = z;
  }

  // check zmax = xmax - ymin
  // check zmin = xmin - ymax

  if (DebugTools::debug_trace_enabled)
    DebugTools::logFile << "\t zmin=" << z_min << " zmax=" << z_max
                        << std::endl;

  if (enable_z_quantization) {
    int z_step = 16;
    int z_limit = 1024;
    z_min = icov::helpers::maths::quantize(z_min, z_step, -z_limit, z_limit);
    z_max = icov::helpers::maths::quantize(z_max, z_step, -z_limit, z_limit);
    if (DebugTools::debug_trace_enabled)
      DebugTools::logFile << "\t After quantization zmin=" << z_min
                          << " zmax=" << z_max << std::endl;
  }

  if (DebugTools::debug_trace_enabled)
    DebugTools::logFile << "\t q=" << q << std::endl;
  if (enable_q_quantization) {
    q = q_values[icov::helpers::maths::quantize(q, q_values)];
    if (DebugTools::debug_trace_enabled)
      DebugTools::logFile << "\t After quantization q=" << q << std::endl;
  }

  // y_min = min(Y[k], 0<=k<N)
  int y_min = *std::min_element(side_info.begin(), side_info.end());

  // y_max = max(Y[k], 0<=k<N)
  int y_max = *std::max_element(side_info.begin(), side_info.end());

  return BitSymbolProvider(z_min, z_max, y_min, y_max, q, lx);
}

BitSymbolProvider::BitSymbolProvider(int z_min, int z_max, int y_min, int y_max,
                                     float q, int l)
    : m_z_min(z_min), m_z_max(z_max), m_y_min(y_min), m_y_max(y_max), m_q0(q),
      m_l(l) {}

BitSymbolProvider::BitSymbolProvider(const SymboleVec &src_symbols)
    : m_z_min(0), m_z_max(0), m_y_min(0), m_y_max(0), m_q0(0), m_l(0),
      m_src_symboles(src_symbols),
      m_src_bits(BitSymbolProvider::compute_bits(src_symbols)) {
  m_l = m_src_bits.size();
}

void BitSymbolProvider::set_si(const SIVec &side_info,
                               bool enable_z_quantization,
                               const std::vector<float> &q_values) {

  const SymboleVec src_symbols = src_symboles();

  ICOV_ASSERT_MSG(src_symbols.size() == side_info.size(),
                  "Src and SI lengths are not equal")

  bool enable_q_quantization = q_values.size() > 0;
  size_t n = src_symbols.size();

  // init intrinsic SI parameters
  float q = 0.0;
  int z_min = std::numeric_limits<int>::max();
  int z_max = std::numeric_limits<int>::min();

  for (size_t i = 0; i < n; i++) {
    int z = src_symbols[i] - side_info[i];

    // q = N0/N
    if (z == 0) {
      q += 1.0 / n;
    }

    // z_min = min(Z[k], 0<=k<N)
    if (z < z_min)
      z_min = z;

    // z_max = max(Z[k], 0<=k<N)
    if (z > z_max)
      z_max = z;
  }

  // check zmax = xmax - ymin
  // check zmin = xmin - ymax

  if (DebugTools::debug_trace_enabled)
    DebugTools::logFile << "\t zmin=" << z_min << " zmax=" << z_max
                        << std::endl;

  if (enable_z_quantization) {
    int z_step = 16;
    int z_limit = 1024;
    z_min = icov::helpers::maths::quantize(z_min, z_step, -z_limit, z_limit);
    z_max = icov::helpers::maths::quantize(z_max, z_step, -z_limit, z_limit);
    if (DebugTools::debug_trace_enabled)
      DebugTools::logFile << "\t After quantization zmin=" << z_min
                          << " zmax=" << z_max << std::endl;
  }

  if (DebugTools::debug_trace_enabled)
    DebugTools::logFile << "\t q=" << q << std::endl;
  if (enable_q_quantization) {
    q = q_values[icov::helpers::maths::quantize(q, q_values)];
    if (DebugTools::debug_trace_enabled)
      DebugTools::logFile << "\t After quantization q=" << q << std::endl;
  }

  // y_min = min(Y[k], 0<=k<N)
  int y_min = *std::min_element(side_info.begin(), side_info.end());

  // y_max = max(Y[k], 0<=k<N)
  int y_max = *std::max_element(side_info.begin(), side_info.end());

  m_z_min = z_min;
  m_z_max = z_max;
  m_y_min = y_min;
  m_y_max = y_max;
  m_q0 = q;
  m_si_symboles = side_info;
  //  m_si_bits = BitSymbolProvider::compute_bits(side_info);
}

void BitSymbolProvider::set_si(const SIVec &si, const params &p) {
  return set_si(si, p.enable_z_quantization, p.q_values);
}

float BitSymbolProvider::z_min() const { return this->m_z_min; }

float BitSymbolProvider::z_max() const { return this->m_z_max; }

float BitSymbolProvider::y_min() const { return this->m_y_min; }

float BitSymbolProvider::y_max() const { return this->m_y_max; }

float BitSymbolProvider::q0() const { return this->m_q0; }

float BitSymbolProvider::q1() const {
  if (m_z_max == m_z_min)
    return 0.0;
  return (1 - m_q0) / (m_z_max - m_z_min);
}

int BitSymbolProvider::z0() const {
  return (m_z_min > 0) ? m_z_min : (m_z_max < 0 ? m_z_max : 0);
}

float BitSymbolProvider::pz(int z) const {
  return (z == z0()) ? m_q0 : ((z < m_z_min || z > m_z_max) ? 0 : q1());
}

float BitSymbolProvider::pz(int zmin, int zmax) const {
  float p = 0;

  for (int z = zmin; z <= zmax; z++) {
    p += this->pz(z);
  }
  return p;
}

int BitSymbolProvider::z_binary_length() const {
  int max = std::max(std::abs(m_z_min), std::abs(m_z_max));
  if (max == 0)
    return 1;
  return std::ceil(std::log2f(max));
}

int BitSymbolProvider::x_max_estimated() const { return m_z_max + m_y_min; }

int BitSymbolProvider::x_min_estimated() const { return m_z_min + m_y_max; }

int BitSymbolProvider::x_binary_length() const {
  return m_l;
  return std::floor(std::log2f(std::max(std::abs(x_min_estimated()),
                                        std::abs(x_max_estimated())))) +
         2;
}

bool BitSymbolProvider::pqkb(bool negative, int abs_predec, int l, int b,
                             int yk, float &p0, float &p1, float &p) const {
  abs_predec = abs_predec << (l - b);

  p0 = 0.0;
  p1 = 0.0;
  p = 0.0f;

  // IMPLEMENTATION FANGPING
  int imax = (1 << (l - b));
  for (int i = 0; i < imax; i++) {
    int z = (abs_predec + i) * (1 - 2 * negative) - yk;

    p0 += this->pz(z);
    p1 += this->pz(z + imax * (1 - 2 * negative));
  }

  // DebugTools::logFile <<  "\t\t\t yk=" << yk << " a=" << p0 <<  " b=" << p1
  // << std::endl;

  // attention b = a+b
  float pb = p1 + p0;

  // DebugTools::logFile <<  "\t\t\t yk=" << yk << std::endl;

  if (DebugTools::debug_trace_enabled) {
    if (pb == 0) {
      std::cerr << "WARNING: pb = 0" << std::endl;

      DebugTools::logFile << "\t\t\t WARNING: pb = (a + b) = 0" << std::endl;
      DebugTools::logFile << "\t\t\t yk=" << yk << " a=" << p0 << " b=" << p1
                          << " imax=" << ((1 << (l - b))) << std::endl;

      DebugTools::logFile << "\t\t\t a = ";
      int imax = (1 << (l - b));
      for (int i = 0; i < imax; i++) {
        int z = (abs_predec + i) * (1 - 2 * negative) - yk;
        DebugTools::logFile << "pz(" << z << ") + ";
      }

      DebugTools::logFile << std::endl << "\t\t\t b = ";

      for (int i = 0; i < imax; i++) {
        int z = (abs_predec + i) * (1 - 2 * negative) - yk +
                imax * (1 - 2 * negative);
        DebugTools::logFile << "pz(" << z << ") + ";
      }

      DebugTools::logFile << std::endl;
    }

    if (std::abs(p0 / pb + p1 / pb - 1) > 0.00001) {
      DebugTools::logFile << "\t\t\t WARNING: sum = " << (p0 / pb + p1 / pb)
                          << " != 1" << std::endl;
    }
  }

  p = pb == 0 ? 0.5 : p0 / pb;

  return pb != 0;

  //  // IMPLEMENTATION SEBASTIEN
  //  int zk0 = (1 - 2 * negative) * (abs_predec << (l - b)) - yk;
  //  int zk1 = (1 << (l - b - 1));
  //  int zk2 = (1 << (l - b));

  //  // std::cout << l <<", " << b << ", " << zk0 << ", " << zk1 << ", " << zk2
  //  <<
  //  // std::endl;

  //  for (int i = 0; i < zk2; i++) {
  //    float pi = this->pz(zk0 + i);
  //    p1 += pi;
  //    if (i < zk1)
  //      p0 += pi;
  //    // std::cout << "i="<<i << " p=" << this->pz(i - yk) << std::endl;
  //  }

  //  //    p1 = std::max(p1, (float)0.000001);

  //  if (p1 == 0)
  //    std::cerr << "WARNING: p1 = 0" << std::endl;

  //  p = p1 == 0 ? 0.5 : p0 / p1;

  //  return p1 != 0;
}

float BitSymbolProvider::pqks(bool negative, int yk) const {

  int imax = (1 << x_binary_length()); // std::max(std::abs(x_min_estimated()),
                                       // std::abs(x_max_estimated()));
  float p0 = 0.0f;
  float p1 = 0.0f;

  for (int i = 0; i < imax; i++) {

    if ((i - yk) != 0 && (-i - yk) != 0) {
      p0 += this->pz(i - yk);
      if (i != 0)
        p1 += this->pz(-i - yk);
    } else if (negative) {
      p1 += this->pz(0);
    } else {
      p0 += this->pz(0);
    }
  }
  return p0 / (p0 + p1);
}
} // namespace icov
