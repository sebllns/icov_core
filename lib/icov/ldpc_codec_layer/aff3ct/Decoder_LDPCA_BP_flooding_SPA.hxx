#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "Decoder_LDPCA_BP_flooding_SPA.hpp"
#include "Tools/Exception/exception.hpp"
#include "helpers.h"
#include "tools/DebugTools.h"
#include <cmath>
#include <limits>
#include <mipp.h>
#include <string>

namespace aff3ct {
namespace module {
template <typename B, typename R>
Decoder_LDPCA_BP_flooding_SPA<B, R>::Decoder_LDPCA_BP_flooding_SPA(
    const int N, const int n_ite, const tools::Sparse_matrix &_H)
    : Decoder_SISO<B, R>(N, N), Decoder_LDPC_BP(N, N, n_ite, _H, false, 1),
      up_rule(
          tools::Update_rule_SPA<R>((unsigned int)_H.get_cols_max_degree())),
      transpose(this->H.get_n_connections()), post(N, -1),
      msg_chk_to_var(this->n_frames,
                     std::vector<R>(this->H.get_n_connections())),
      msg_var_to_chk(this->n_frames,
                     std::vector<R>(this->H.get_n_connections())),
      values(_H.get_cols_max_degree()) {

  const std::string name = "Decoder_LDPCA_BP_flooding_SPA";
  this->set_name(name);

  mipp::vector<unsigned char> connections(this->H.get_n_rows(), 0);

  const auto &msg_chk_to_var_id = this->H.get_col_to_rows();
  const auto &msg_var_to_chk_id = this->H.get_row_to_cols();

  auto k = 0;
  for (auto i = 0; i < (int)msg_chk_to_var_id.size(); i++) {
    for (auto j = 0; j < (int)msg_chk_to_var_id[i].size(); j++) {
      auto var_id = msg_chk_to_var_id[i][j];

      auto branch_id = 0;
      for (auto ii = 0; ii < (int)var_id; ii++)
        branch_id += (int)msg_var_to_chk_id[ii].size();
      branch_id += connections[var_id];
      connections[var_id]++;

      if (connections[var_id] > (int)msg_var_to_chk_id[var_id].size()) {
        std::stringstream message;
        message << "'connections[var_id]' has to be equal or smaller than "
                   "'msg_var_to_chk_id[var_id].size()' "
                << "('var_id' = " << var_id
                << ", 'connections[var_id]' = " << connections[var_id]
                << ", 'msg_var_to_chk_id[var_id].size()' = "
                << msg_var_to_chk_id[var_id].size() << ").";
        throw tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
      }

      this->transpose[k] = branch_id;
      k++;
    }
  }

  this->reset();
}

template <typename B, typename R>
Decoder_LDPCA_BP_flooding_SPA<B, R> *
Decoder_LDPCA_BP_flooding_SPA<B, R>::clone() const {
  auto m = new Decoder_LDPCA_BP_flooding_SPA(*this);
  m->deep_copy(*this);
  return m;
}

template <typename B, typename R>
void Decoder_LDPCA_BP_flooding_SPA<B, R>::_reset(const size_t frame_id) {
  std::fill(this->msg_chk_to_var[frame_id].begin(),
            this->msg_chk_to_var[frame_id].end(), (R)0);
}

template <typename B, typename R>
int Decoder_LDPCA_BP_flooding_SPA<B, R>::_decode_siso(const R *Y_N1, R *Y_N2,
                                                      const size_t frame_id) {
  this->_decode(Y_N1, frame_id);

  // prepare for next round by processing extrinsic information
  for (auto v = 0; v < this->N; v++)
    Y_N2[v] = this->post[v] - Y_N1[v];

  return 0;
}

template <typename B, typename R>
int Decoder_LDPCA_BP_flooding_SPA<B, R>::_decode_siho(const R *Y_N, B *V_K,
                                                      const size_t frame_id) {

  //    // memory zones initialization
  //    if (this->init_flag)
  //    {
  //        std::fill(this->msg_chk_to_var[frame_id].begin(),
  //        this->msg_chk_to_var[frame_id].end(), (R)0);

  //        if (frame_id == Decoder_SIHO<B,R>::n_frames -1)
  //            this->init_flag = false;
  //    }

  // actual decoding
  this->_decode(Y_N, frame_id);

  // take the hard decision
  for (auto v = 0; v < this->K; v++) {
    const auto k = v;
    V_K[v] = !(this->post[k] >= 0);
  }

  return 0;
}

template <typename B, typename R>
int Decoder_LDPCA_BP_flooding_SPA<B, R>::_decode_siho_cw(
    const R *Y_N, B *V_N, const size_t frame_id) {
  //	auto t_load = std::chrono::steady_clock::now(); //
  //----------------------------------------------------------- LOAD
  // memory zones initialization
  //	auto d_load = std::chrono::steady_clock::now() - t_load;

  //	auto t_decod = std::chrono::steady_clock::now(); //
  //-------------------------------------------------------- DECODE
  // actual decoding
  this->_decode(Y_N, frame_id);
  //	auto d_decod = std::chrono::steady_clock::now() - t_decod;

  //	auto t_store = std::chrono::steady_clock::now(); //
  //--------------------------------------------------------- STORE
  tools::hard_decide(this->post.data(), V_N, this->N);
  //	auto d_store = std::chrono::steady_clock::now() - t_store;

  //	(*this)[dec::tsk::decode_siho_cw].update_timer(dec::tm::decode_siho_cw::load,
  // d_load);
  //	(*this)[dec::tsk::decode_siho_cw].update_timer(dec::tm::decode_siho_cw::decode,
  // d_decod);
  //	(*this)[dec::tsk::decode_siho_cw].update_timer(dec::tm::decode_siho_cw::store,
  // d_store);
  return 0;
}

template <typename B, typename R>
void Decoder_LDPCA_BP_flooding_SPA<B, R>::_decode(const R *Y_N,
                                                  const int frame_id) {
  int sameCount = 0;
  this->up_rule.begin_decoding(this->n_ite);

  auto ite = 0;
  for (; ite < this->n_ite; ite++) {

    this->up_rule.begin_ite(ite);
    this->_initialize_var_to_chk(Y_N, this->msg_chk_to_var[frame_id],
                                 this->msg_var_to_chk[frame_id]);

    this->_decode_single_ite(this->msg_var_to_chk[frame_id],
                             this->msg_chk_to_var[frame_id]);
    this->up_rule.end_ite();

    if (this->enable_syndrome && ite != this->n_ite - 1) {
      this->_compute_post(Y_N, this->msg_chk_to_var[frame_id], this->post);
      if (this->check_syndrome_soft(this->post.data()))
        break;
    }
  }
  if (ite == this->n_ite)
    this->_compute_post(Y_N, this->msg_chk_to_var[frame_id], this->post);

  this->up_rule.end_decoding();
}

template <typename B, typename R>
void Decoder_LDPCA_BP_flooding_SPA<B, R>::_initialize_var_to_chk(
    const R *Y_N, const std::vector<R> &msg_chk_to_var,
    std::vector<R> &msg_var_to_chk) {
  auto *msg_chk_to_var_ptr = msg_chk_to_var.data();
  auto *msg_var_to_chk_ptr = msg_var_to_chk.data();

  const auto n_var_nodes = (int)this->H.get_n_rows();
  ;
  for (auto v = 0; v < n_var_nodes; v++) {
    const auto var_degree = (int)this->H.get_row_to_cols()[v].size();

    auto sum_msg_chk_to_var = (R)0;
    for (auto c = 0; c < var_degree; c++)
      sum_msg_chk_to_var += msg_chk_to_var_ptr[c];

    const auto tmp = Y_N[v] + sum_msg_chk_to_var;

    for (auto c = 0; c < var_degree; c++)
      msg_var_to_chk_ptr[c] = tmp - msg_chk_to_var_ptr[c];

    msg_chk_to_var_ptr += var_degree;
    msg_var_to_chk_ptr += var_degree;
  }
}

template <typename B, typename R>
void Decoder_LDPCA_BP_flooding_SPA<B, R>::get_llr_order(
    std::vector<size_t> &idx) const {
  icov::sort_indexes_abs(this->post, idx);
}

template <typename B, typename R>
void Decoder_LDPCA_BP_flooding_SPA<B, R>::_decode_single_ite(
    const std::vector<R> &msg_var_to_chk, std::vector<R> &msg_chk_to_var) {
  const auto n_branches = (int)this->H.get_n_connections();
  auto transpose_ptr = this->transpose.data();

  mipp::Reg<R> r_tmp;
  const mipp::Reg<R> r_1 = (R)1.0;
  const auto vec_loop_size = (n_branches / mipp::N<R>()) * mipp::N<R>();
  for (auto b = 0; b < vec_loop_size; b += mipp::N<R>()) {
    r_tmp = &msg_var_to_chk[b];
    // tanh(1/2 * t) = (exp(t) - 1) / (exp(t) + 1)
    r_tmp = mipp::exp(r_tmp);
    r_tmp = (r_tmp - r_1) / (r_tmp + r_1);
    r_tmp.storeu(&msg_chk_to_var[b]);
  }

  // tail loop to compute the remaining elements
  for (auto b = vec_loop_size; b < n_branches; b++)
    msg_chk_to_var[b] = (R)std::tanh((R)0.5 * msg_var_to_chk[b]);

  double prod_check = 0;
  double val_check = 0;

  // flooding scheduling
  const auto n_chk_nodes = (int)this->H.get_n_cols();
  for (auto c = 0; c < n_chk_nodes; c++) {

    const auto chk_degree = (int)this->H.get_col_to_rows()[c].size();

    // LDPCA EDIT HERE WITH 1 - 2*s
    auto prod = (R)((m_syndrome->at(c)) ? -1.0 : 1.0);

    for (auto v = 0; v < chk_degree; v++) {
      this->values[v] = msg_chk_to_var[transpose_ptr[v]];
      // std::cout<<this->values[v]<<", ";
      prod *= this->values[v];
    }

    for (auto v = 0; v < chk_degree; v++) {
      auto den = this->values[v];
      auto val = prod;

      if (den == 0) {
        // FIXME: It seems there is a division by zero in some cases
        //#if ICOV_CORE_DEBUG_TRACE_ENABLED
        //        ICOV_LOG(trace) << "WARNING den is zero and prod is " <<
        //        prod
        //                                  << std::endl;
        //#endif
        den = std::numeric_limits<R>::epsilon();
      }

      val /= den;
      val = (std::abs(val) < (R)1.0)
                ? val
                : ((R)1.0 - std::numeric_limits<R>::epsilon()) *
                      (val > 0 ? 1 : -1);
      msg_chk_to_var[transpose_ptr[v]] = (R)val;
    }

    transpose_ptr += chk_degree;
  }

  for (auto b = 0; b < vec_loop_size; b += mipp::N<R>()) {
    r_tmp = &msg_chk_to_var[b];
    // 2 * atanh(t) = log((1 + t) / (1 - t)
    r_tmp = mipp::log((r_1 + r_tmp) / (r_1 - r_tmp));
    r_tmp.storeu(&msg_chk_to_var[b]);
  }
  // tail loop to compute the remaining elements
  for (auto b = vec_loop_size; b < n_branches; b++) {
    msg_chk_to_var[b] = (R)2.0 * (R)std::atanh(msg_chk_to_var[b]);
  }
}

template <typename B, typename R>
void Decoder_LDPCA_BP_flooding_SPA<B, R>::_compute_post(
    const R *Y_N, const std::vector<R> &msg_chk_to_var, std::vector<R> &post) {
  // compute the a posteriori info
  const auto *msg_chk_to_var_ptr = msg_chk_to_var.data();
  const auto n_var_nodes = (int)this->H.get_n_rows();
  ;
  for (auto v = 0; v < n_var_nodes; v++) {
    const auto var_degree = (int)this->H.get_row_to_cols()[v].size();

    auto sum_msg_chk_to_var = (R)0;
    for (auto c = 0; c < var_degree; c++)
      sum_msg_chk_to_var += msg_chk_to_var_ptr[c];

    // filling the output
    post[v] = Y_N[v] + sum_msg_chk_to_var;

    msg_chk_to_var_ptr += var_degree;
  }
}

template <typename B, typename R>
void Decoder_LDPCA_BP_flooding_SPA<B, R>::set_n_frames(const size_t n_frames) {
  const auto old_n_frames = this->get_n_frames();
  if (old_n_frames != n_frames) {
    Decoder_SISO<B, R>::set_n_frames(n_frames);

    const auto vec_size = this->msg_chk_to_var[0].size();
    const auto old_msg_chk_to_var_size = this->msg_chk_to_var.size();
    const auto new_msg_chk_to_var_size =
        (old_msg_chk_to_var_size / old_n_frames) * n_frames;
    this->msg_chk_to_var.resize(new_msg_chk_to_var_size,
                                std::vector<R>(vec_size));

    const auto vec_size2 = this->msg_var_to_chk[0].size();
    const auto old_msg_var_to_chk_size = this->msg_var_to_chk.size();
    const auto new_msg_var_to_chk_size =
        (old_msg_var_to_chk_size / old_n_frames) * n_frames;
    this->msg_var_to_chk.resize(new_msg_var_to_chk_size,
                                std::vector<R>(vec_size2));
  }
}
} // namespace module
} // namespace aff3ct
