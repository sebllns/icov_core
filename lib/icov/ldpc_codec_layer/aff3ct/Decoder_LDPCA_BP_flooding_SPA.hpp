#ifndef Decoder_LDPCA_BP_flooding_SPA_HPP_
#define Decoder_LDPCA_BP_flooding_SPA_HPP_

#include "Module/Decoder/Decoder_SISO.hpp"
#include "Module/Decoder/LDPC/BP/Decoder_LDPC_BP.hpp"
#include "Tools/Algo/Matrix/Sparse_matrix/Sparse_matrix.hpp"
#include "Tools/Code/LDPC/Update_rule/SPA/Update_rule_SPA.hpp"
#include "FastArray.h"
#include <cstdint>
#include <fstream>
#include <vector>

namespace icov {
namespace ldpc_codec_layer {
template <typename B = int, typename R = float>
class Decoder_LDPCA_BP_flooding_SPA : public aff3ct::module::Decoder_SISO<B, R>,
                                      public aff3ct::module::Decoder_LDPC_BP {

protected:
  std::vector<R> values;
  icov::Syndrome *m_syndrome;

  aff3ct::tools::Update_rule_SPA<R> up_rule;

  std::vector<uint32_t> transpose;
  std::vector<R> post; // a posteriori information
  std::vector<std::vector<R>>
      msg_chk_to_var; // check    nodes to variable nodes messages
  std::vector<std::vector<R>>
      msg_var_to_chk; // variable nodes to check    nodes messages

public:
  Decoder_LDPCA_BP_flooding_SPA(const int N, const int n_ite,
                                const aff3ct::tools::Sparse_matrix &H);
  virtual ~Decoder_LDPCA_BP_flooding_SPA() = default;

  void get_llr_order(std::vector<size_t> &idx) const;

  void setSyndrome(icov::Syndrome *synd) { m_syndrome = synd; }

  virtual Decoder_LDPCA_BP_flooding_SPA<B, R> *clone() const;
  virtual void set_n_frames(const size_t n_frames);

protected:
  void _reset(const size_t frame_id);

  int _decode_siso(const R *Y_N1, R *Y_N2, const size_t frame_id);
  int _decode_siho(const R *Y_N, B *V_K, const size_t frame_id);
  int _decode_siho_cw(const R *Y_N, B *V_N, const size_t frame_id);

  void _decode(const R *Y_N, const int frame_id);
  void _initialize_var_to_chk(const R *Y_N,
                              const std::vector<R> &msg_chk_to_var,
                              std::vector<R> &msg_var_to_chk);
  void _decode_single_ite(const std::vector<R> &msg_var_to_chk,
                          std::vector<R> &msg_chk_to_var);
  void _compute_post(const R *Y_N, const std::vector<R> &msg_chk_to_var,
                     std::vector<R> &post);
};

} // namespace ldpc_codec_layer
} // namespace icov

#include "Decoder_LDPCA_BP_flooding_SPA.hxx"

#endif /* Decoder_LDPCA_BP_flooding_SPA_HPP_ */
