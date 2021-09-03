#ifndef Decoder_LDPCA_BP_flooding_SPA_HPP_
#define Decoder_LDPCA_BP_flooding_SPA_HPP_

#include <cstdint>
#include <vector>
#include <fstream>

#include "Tools/Code/LDPC/Update_rule/SPA/Update_rule_SPA.hpp"
#include "Tools/Algo/Matrix/Sparse_matrix/Sparse_matrix.hpp"
#include "Module/Decoder/LDPC/BP/Flooding/Decoder_LDPC_BP_flooding.hpp"

#include "common.h"

namespace aff3ct
{
namespace module
{
template <typename B = int, typename R = float>
class Decoder_LDPCA_BP_flooding_SPA : public Decoder_LDPC_BP_flooding<B,R,tools::Update_rule_SPA<R>>
{
protected:
	std::vector<R> values;
    std::vector<R> LLR_overall;
    icov::Syndrome *m_syndrome;


public:
    Decoder_LDPCA_BP_flooding_SPA(const int N, const int n_ite,
                                  const tools::Sparse_matrix &H);
	virtual ~Decoder_LDPCA_BP_flooding_SPA() = default;

    std::vector<size_t> get_llr_order(std::vector<size_t> &idx) const;

    void setSyndrome(icov::Syndrome *synd) {m_syndrome = synd;}

protected:
	void _decode_single_ite(const std::vector<R> &msg_var_to_chk, std::vector<R> &msg_chk_to_var);
    void _decode_siho   (const R *Y_N,  B *V_K,  const int frame_id);
    void _decode(const R *Y_N, const int frame_id);

    void _initialize_var_to_chk(const R *Y_N, const std::vector<R> &msg_chk_to_var, std::vector<R> &msg_var_to_chk);



};

}
}

#include "Decoder_LDPCA_BP_flooding_SPA.hxx"

#endif /* Decoder_LDPCA_BP_flooding_SPA_HPP_ */
