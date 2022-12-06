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

void BitSymbolProvider::inject_bitplane(SymboleVec &output, const BitVec &input,
                                        int shift) {

  ICOV_ASSERT_MSG(output.size() == input.size(),
                  "Vectors must be same size for this operation");

  for (size_t i = 0; i < output.size(); ++i) {
    if (shift < 0) /*apply sign bitplane*/
    {
      if (input[i])
        output[i] *= -1;
    } else {
      output[i] = (output[i] << shift) + input[i];
    }
  }
}

BitSymbolProvider::BitSymbolProvider(int n)
    : m_src_symboles(n, 0),
      m_src_bits(1 /*we need to save only the sign bitplane to decode*/,
                 BitVec(n, 0)) {}

BitSymbolProvider::BitSymbolProvider(const SymboleVec &src_symbols)
    : m_l(0), m_src_symboles(src_symbols),
      m_src_bits(BitSymbolProvider::compute_bits(src_symbols)) {
  m_l = m_src_bits.size();
}

void BitSymbolProvider::set_si(const SIVec &side_info,
                               bool enable_z_quantization,
                               const std::vector<float> &q_values) {

  auto &src_symbols = m_src_symboles;

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
    ICOV_LOG(trace) << "\t zmin=" << z_min << " zmax=" << z_max << std::endl;

  if (enable_z_quantization) {
    int z_step = 16;
    int z_limit = 1024;
    z_min = icov::helpers::maths::quantize(z_min, z_step, -z_limit, z_limit);
    z_max = icov::helpers::maths::quantize(z_max, z_step, -z_limit, z_limit);
    if (DebugTools::debug_trace_enabled)
      ICOV_LOG(trace) << "\t After quantization zmin=" << z_min
                      << " zmax=" << z_max << std::endl;
  }

  if (DebugTools::debug_trace_enabled)
    ICOV_LOG(trace) << "\t q=" << q << std::endl;
  if (enable_q_quantization) {
    q = q_values[icov::helpers::maths::quantize(q, q_values)];
    if (DebugTools::debug_trace_enabled)
      ICOV_LOG(trace) << "\t After quantization q=" << q << std::endl;
  }

  m_pz_params = Pz(z_min, z_max, q);
  m_si_symboles = side_info;
}

void BitSymbolProvider::set_si(const SIVec &si, const params &p) {
  set_si(si, p.enable_z_quantization, p.q_values);
}

void BitSymbolProvider::set_si(const SIVec &si, int z_min, int z_max, float q,
                               int l) {

  ICOV_ASSERT_MSG(si.size() == m_src_symboles.size(),
                  "SI must be same size as source");
  m_pz_params = Pz(z_min, z_max, q);
  m_si_symboles = si;
  m_l = l;
  std::fill(m_src_symboles.begin(), m_src_symboles.end(), 0);
  // std::fill(m_src_bits[0].begin(), m_src_bits[0].end(), 0);

  m_src_bits = Bitplanes(l, BitVec(m_src_symboles.size(), 0));
}

float BitSymbolProvider::z_min() const { return m_pz_params.zmin; }

float BitSymbolProvider::z_max() const { return m_pz_params.zmax; }

float BitSymbolProvider::q0() const { return m_pz_params.q0; }

float BitSymbolProvider::q1() const { return m_pz_params.q1; }

int BitSymbolProvider::z0() const { return m_pz_params.z0; }

float BitSymbolProvider::pz(int z) const {
  return (z == z0()) ? q0() : ((z < z_min() || z > z_max()) ? 0.0 : q1());
}

float BitSymbolProvider::pz(int zmin, int zmax) const {
  float p = 0;

  for (int z = zmin; z <= zmax; z++) {
    p += this->pz(z);
  }
  return p;
}

int BitSymbolProvider::x_binary_length() const { return m_l; }

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

  // ICOV_LOG(trace) <<  "\t\t\t yk=" << yk << " a=" << p0 <<  " b=" << p1
  // << std::endl;

  // attention b = a+b
  float pb = p1 + p0;

  // ICOV_LOG(trace) <<  "\t\t\t yk=" << yk << std::endl;

  if (DebugTools::debug_trace_enabled) {
    if (pb == 0) {

      ICOV_LOG(trace) << "\t\t\t WARNING: pb = (a + b) = 0" << std::endl;
      ICOV_LOG(trace) << "\t\t\t yk=" << yk << " a=" << p0 << " b=" << p1
                      << " imax=" << ((1 << (l - b))) << std::endl;

      ICOV_LOG(trace) << "\t\t\t a = ";
      int imax = (1 << (l - b));
      for (int i = 0; i < imax; i++) {
        int z = (abs_predec + i) * (1 - 2 * negative) - yk;
        ICOV_LOG(trace) << "pz(" << z << ") + ";
      }

      ICOV_LOG(trace) << std::endl << "\t\t\t b = ";

      for (int i = 0; i < imax; i++) {
        int z = (abs_predec + i) * (1 - 2 * negative) - yk +
                imax * (1 - 2 * negative);
        ICOV_LOG(trace) << "pz(" << z << ") + ";
      }

      ICOV_LOG(trace) << std::endl;
    } else {

      if (std::abs(p0 / pb + p1 / pb - 1) > 0.00001) {
        ICOV_LOG(trace) << "\t\t\t WARNING: sum = " << (p0 / pb + p1 / pb)
                        << " != 1" << std::endl;
      }
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

float BitSymbolProvider::pqks(int yk) const {
  int imax = (1 << x_binary_length()); // std::max(std::abs(x_min_estimated()),
                                       // std::abs(x_max_estimated()));
  float p0 = 0.0f;
  float p1 = 0.0f;

  // IMPLEMENTATION SEBASTIEN
  for (int i = 0; i < imax; i++) {
    p0 += this->pz(i - yk);
    if (i != 0)
      p1 += this->pz(-i - yk);
  }
  return p0 / (p0 + p1);
}

void BitSymbolProvider::injectBitplane(uint i, BitVec x) {
  m_src_bits[i] = x;

  if (i == 0) {
    // save sign
    m_src_bits[0] = x;
  } else {
    // apply decoded bitplane
    BitSymbolProvider::inject_bitplane(m_src_symboles, x, 1);

    // if last apply sign
    if (i == ((uint)m_l - 1))
      BitSymbolProvider::inject_bitplane(m_src_symboles, m_src_bits[0], -1);
  }
}
} // namespace icov
