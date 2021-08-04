#ifndef Decoder_LDPCA_BP_flooding_SPA_HPP_
#define Decoder_LDPCA_BP_flooding_SPA_HPP_

#include <cstdint>
#include <vector>
#include <fstream>

#include "Tools/Code/LDPC/Update_rule/SPA/Update_rule_SPA.hpp"
#include "Tools/Algo/Matrix/Sparse_matrix/Sparse_matrix.hpp"
#include "Module/Decoder/LDPC/BP/Flooding/Decoder_LDPC_BP_flooding.hpp"

#include "common_types.h"

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
    const icov::Syndrome *m_syndrome;


public:
	Decoder_LDPCA_BP_flooding_SPA(const int K, const int N, const int n_ite,
                                  const tools::Sparse_matrix &H,
                                  const std::vector<uint32_t> &info_bits_pos,
                                  const icov::Syndrome *synd,
                                  const bool enable_syndrome = true,
                                  const int syndrome_depth = 1,
                                  const int n_frames = 1);
	virtual ~Decoder_LDPCA_BP_flooding_SPA() = default;

//    void setSyndrome(const icov::Syndrome& syndrome)
//    {
//        m_syndrome = syndrome;
//    }


protected:
	void _decode_single_ite(const std::vector<R> &msg_var_to_chk, std::vector<R> &msg_chk_to_var);
    void _decode_siho   (const R *Y_N,  B *V_K,  const int frame_id);
    void _decode(const R *Y_N, const int frame_id);

    void initialize_var_to_chk(const R *Y_N, const std::vector<R> &msg_chk_to_var, std::vector<R> &msg_var_to_chk);



};

}
}

#include "Decoder_LDPCA_BP_flooding_SPA.hxx"

#endif /* Decoder_LDPCA_BP_flooding_SPA_HPP_ */
