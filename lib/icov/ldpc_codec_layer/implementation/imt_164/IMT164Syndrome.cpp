#include "IMT164Syndrome.h"

#include "helpers.h"
#include "implementation/imt_164/IMT164SyndromeTools.h"
#include "tools/DebugTools.h"

namespace icov {
namespace ldpc_codec_layer {

IMT164SyndromeManager::IMT164SyndromeManager(
    const IMT164SyndromeTools &tools,
    const std::shared_ptr<const IMT164MatrixProvider> &matrix_pvdr)
    : ISyndromeManager(tools.get_n()), m_tools(tools),
      m_matrixPvdr(matrix_pvdr), accumulated_syndrome(nullptr) {

  this->reset_doping();

  //  ICOV_ASSERT_MSG(m_min_m >= get_m_level(m_max_level), "Min m bad value");
  //  ICOV_ASSERT_MSG(m_max_m <= get_m_level(m_min_level), "Max m bad value");
}

IMT164SyndromeManager::~IMT164SyndromeManager() {
  if (accumulated_syndrome != nullptr)
    delete[] accumulated_syndrome;
}

int IMT164SyndromeManager::get_m_max() const { return m_tools.get_m_max(); }

int IMT164SyndromeManager::get_m_min() const { return m_tools.get_m_min(); }

int IMT164SyndromeManager::get_numcode(int m) const {
  return m_tools.get_numcode(m);
}

///
/// \brief Get the height of the decoding matrix and length of associated
/// syndrome from a given numcode index
///
/// \param numcode index
///
/// \return Length of associated syndrome that is also the height of the
/// decoding matrix
///
int IMT164SyndromeManager::get_m(int numcode) const {
  return m_tools.get_m(numcode);
}

///
/// \brief Get type identifier of the coder class
///
/// \return Type of the coder (string)
///
std::string IMT164SyndromeManager::get_type() const { return "imt164"; }

///
/// \brief Reset the doping order
///
// void IMT164SyndromeManager::reset_doping() {

//}

void IMT164SyndromeManager::compute_doping_order(LLRProvider &llr_provider,
                                                 Syndrome &syndrome) {
  llr_provider.init_doping();
  // Aff3ctDecoder dec(0);
  // BitVec o_decoded(this->get_n());
  // int numcode = this->get_m_max();
  // dec.decode_doping(o_decoded, doping_idx, this->get_n(), llr_provider.llr(),
  //                syndrome, h[this->get_level_count() - 1]);
}

void IMT164SyndromeManager::init_accumulation(const BitVec &source_x) {
  // DebugTools::write_vector("./src.txt", source_x, ' ');
  ICOV_TRACE << __func__;
  reset_doping();

  if (accumulated_syndrome == nullptr) {
    accumulated_syndrome = new icov::bit[this->get_m_max()];
  }

  int offset = 0;

  //    std::cout << "ACCUM" << std::endl;

  for (int k = 0; k < m_matrixPvdr->get_level_count(); k++) {
    int m = m_matrixPvdr->get_m_level(m_matrixPvdr->get_level(k));
    int count = k > 0 ? m / 2 : m;

    //        std::cout << h[k].get_n_rows() << "x" << h[k].get_n_cols() <<
    //        std::endl;

    // std::stringstream sstr;
    // sstr << h[k].transpose();
    // DebugTools::write_str("./h_"+std::to_string(m), sstr.str());

    for (int i = 0; i < count; i++) {
      int col =
          k > 0 ? m_matrixPvdr->mapping_file_index(k - 1, count - i - 1, 0) : i;

      // compute syndrome = h * x
      icov::bit val = 0;
      for (const auto &vn : m_matrixPvdr->get_decoding_matrix_col(k, col)) {
        val ^= source_x[vn];
      }

      int s = i + offset;

      //            std::cout << "s" << s + 1 << " = s" << (int)m_levels[k] <<
      //            "_" << col+1 <<" = " << val << std::endl;

      accumulated_syndrome[s] = val;
    }

    offset += count;
  }

  // DebugTools::write_str("./acc.txt",
  // DebugTools::array_tostring(accumulated_syndrome, offset, "acc"));

  icov::helpers::assert::equal(offset, get_m_max(), "offset != n/2");

  // init cache for doping
  doping_synd = Syndrome(get_m_max());
  for (int i = 0; i < get_m_max(); i++) {
    // compute syndrome = h * x
    icov::bit val = 0;
    for (const auto &vn : m_matrixPvdr->get_decoding_matrix_col(
             m_matrixPvdr->get_level_count() - 1, i)) {
      val ^= source_x[vn];
    }

    doping_synd[i] = val;
  }
}

void IMT164SyndromeManager::compute_deacc(Syndrome &o_syndrome,
                                          const Syndrome &acc_syndrome,
                                          int numcode,
                                          LLRProvider &llr_provider) {

  int m = get_m(numcode);
  // int n = get_n();

  // std::cout << "compute_deacc " << m << std::endl;

  o_syndrome.resize(std::min(m, get_m_max()));

  for (int i = 0; i < std::min(m, get_m_max()); i++) {
    o_syndrome[i] = 0;

    //        std::cout << i+1 << "=";

    for (const auto &k : m_matrixPvdr->decode_syndrome_indexes(numcode, i)) {
      // std::cout << k << "(" << acc_syndrome[k-1] << ")";
      o_syndrome[i] ^= acc_syndrome[k - 1];
    }

    //        std::cout << std::endl;
  }

  // CHECK SYNDROME
  //    auto &s = synd_dec[std::min(numcode, (int)synd_dec.size()-1)];

  //    for (int i = 0; i < std::min(m, get_m_max()); i++)
  //    {
  //        if(o_syndrome[i] != synd_dec[std::min(numcode,
  //        (int)synd_dec.size()-1)][i])
  //        {
  //            std::cout << "error synd " << i + 1<< std::endl;
  //        }
  //    }

  //    std::cout << "acc: "<< acc_syndrome.to_string() << std::endl;
  //    std::cout << "dec: " << o_syndrome.to_string() << std::endl;
  //    std::cout << "gt: " << s.to_string() << std::endl;
  //    std::cout << "--" << std::endl;

  // doping for rate > 0.5
  if (m > get_m_max()) {

#if ICOV_CORE_DEBUG_TRACE_ENABLED
    ICOV_TRACE << "doping";
#endif

    // order the llr values for rate = 0.5
    compute_doping_order(llr_provider, doping_synd);
    int doping_idx = 0;

    o_syndrome.resize(m);

    // put all m-m_12 lowest LLR to the max value (keeping sign)
    for (int i = get_m_max(); i < m; i++) {
      o_syndrome[i] = acc_syndrome[i];
      llr_provider.apply_doping(doping_idx, o_syndrome[i]);
      doping_idx++;
      //      LLRVal llr_val = (o_syndrome[i] ? -1.0 : 1.0) *
      //      llr_provider.limit_max(); llr_provider[doping_idx[i -
      //      get_m_max()]] = llr_val;
    }
  }
}

void IMT164SyndromeManager::compute_stored(Syndrome &o_stored_syndrome,
                                           int numcode,
                                           LLRProvider &llr_provider,
                                           const BitVec &src) {
  int m = o_stored_syndrome.size();
  int m_target = this->get_m(numcode);

  int m_max = get_m_max();
  int m_min = get_m_min();

  // intial syndrome is syndrome18
  if (m_target == m_min || m < m_min) {
    reset_doping();
    o_stored_syndrome.resize(m_min);
    for (int i = 0; i < m_min; i++) {
      o_stored_syndrome[i] = this->accumulated_syndrome[i];
      //            std::cout << "stored_"<<i+1<<std::endl;
    }

    m = m_min;
  }

  while (++m <= m_target) {
    o_stored_syndrome.resize(m);

    if (m <= m_max) {
      //            std::cout << "stored_"<<m<<std::endl;
      // incremental syndrome
      o_stored_syndrome[m - 1] = this->accumulated_syndrome[m - 1];
    } else {
      // doping when rate > 0.5

      // if (init_rateless) {
      compute_doping_order(llr_provider, doping_synd);
      //}

      o_stored_syndrome[m - 1] = src[llr_provider.doping_order(m - 1 - m_max)];
    }
  }
}
} // namespace ldpc_codec_layer
} // namespace icov
