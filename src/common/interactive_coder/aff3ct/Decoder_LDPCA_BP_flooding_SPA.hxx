#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <string>
#include <limits>
#include <cmath>
#include <mipp.h>

#include "Tools/Exception/exception.hpp"
#include "Decoder_LDPCA_BP_flooding_SPA.hpp"

namespace aff3ct
{
namespace module
{
template <typename B, typename R>
Decoder_LDPCA_BP_flooding_SPA<B,R>
::Decoder_LDPCA_BP_flooding_SPA(const int K, const int N, const int n_ite,
                                const tools::Sparse_matrix &_H,
                                const std::vector<uint32_t> &info_bits_pos,
                                const icov::Syndrome *synd,
                                const bool enable_syndrome,
                                const int syndrome_depth,
                                const int n_frames)
: Decoder(K, N, n_frames, 1),
  Decoder_LDPC_BP_flooding<B,R,tools::Update_rule_SPA<R>>(K, N, n_ite, _H, info_bits_pos,
                                                          tools::Update_rule_SPA<R>((unsigned int)_H.get_cols_max_degree()),
                                                          enable_syndrome, syndrome_depth, n_frames),
  values(_H.get_cols_max_degree()),
  LLR_overall(_H.get_n_rows()),
  m_syndrome(synd)
 //, _syndrome(_H.get_n_rows())
{
	const std::string name = "Decoder_LDPCA_BP_flooding_SPA";
	this->set_name(name);
}

template <typename B, typename R>
void Decoder_LDPCA_BP_flooding_SPA<B,R>
::_decode_siho(const R *Y_N, B *V_K, const int frame_id)
{

//	auto t_load = std::chrono::steady_clock::now(); // ----------------------------------------------------------- LOAD
    // memory zones initialization
    if (this->init_flag)
    {
        std::fill(this->msg_chk_to_var[frame_id].begin(), this->msg_chk_to_var[frame_id].end(), (R)0);

        if (frame_id == Decoder_SIHO<B,R>::n_frames -1)
            this->init_flag = false;
    }
//	auto d_load = std::chrono::steady_clock::now() - t_load;

//	auto t_decod = std::chrono::steady_clock::now(); // -------------------------------------------------------- DECODE
    // actual decoding
    this->_decode(Y_N, frame_id);

    //write_vector("msg_var_to_chk", this->msg_var_to_chk[frame_id]);
//	auto d_decod = std::chrono::steady_clock::now() - t_decod;

//	auto t_store = std::chrono::steady_clock::now(); // --------------------------------------------------------- STORE
    // take the hard decision
    for (auto v = 0; v < this->K; v++)
    {
        //const auto k = this->info_bits_pos[v];
        const auto k = v;
        V_K[v] = !(this->post[k] >= 0);
    }
//	auto d_store = std::chrono::steady_clock::now() - t_store;

//	(*this)[dec::tsk::decode_siho].update_timer(dec::tm::decode_siho::load,   d_load);
//	(*this)[dec::tsk::decode_siho].update_timer(dec::tm::decode_siho::decode, d_decod);
//	(*this)[dec::tsk::decode_siho].update_timer(dec::tm::decode_siho::store,  d_store);
}








template <typename B, typename R>
void Decoder_LDPCA_BP_flooding_SPA<B,R>
::_decode(const R *Y_N, const int frame_id)
{

    int     sameCount = 0;


//    std::cout << "\n\n_decode\n================================================================================================================\n===================================================================================================\n\n";

  //  getchar();

//    std::cout << "frame_id = "  << frame_id <<std::endl;

    this->up_rule.begin_decoding(this->n_ite);

    auto ite = 0;
    for (; ite < this->n_ite; ite++)
    {
//            std::cout <<"iteration: "<<ite+1<<"/"<< this->n_ite<< "\n================================================================================================================\n===================================================================================================\n\n";


        this->up_rule.begin_ite(ite);
        this->_initialize_var_to_chk(Y_N, this->msg_chk_to_var[frame_id], this->msg_var_to_chk[frame_id]);

//        write_vector("_decode_msg_var_to_chk_001", this->msg_var_to_chk[frame_id]);
//        write_vector("_decode_msg_chk_to_var_001", this->msg_chk_to_var[frame_id]);


//        std::cout <<"decode iteration: "<<ite+1<<"/"<< this->n_ite<< "\n================================================================================================================\n===================================================================================================\n\n";

//        getchar();

        this->_decode_single_ite(this->msg_var_to_chk[frame_id], this->msg_chk_to_var[frame_id]);
        this->up_rule.end_ite();


//        write_vector("_decode_msg_var_to_chk_002", this->msg_var_to_chk[frame_id]);
//        write_vector("_decode_msg_chk_to_var_002", this->msg_chk_to_var[frame_id]);


        if (this->enable_syndrome && ite != this->n_ite -1)
        {
            this->_compute_post(Y_N, this->msg_chk_to_var[frame_id], this->post);
            if (this->check_syndrome_soft(this->post.data()))
                break;
        }



            //Step 3: test convergence and syndrome condition

//            int l = 0;
//            for (int k = 0; k < this->N; k++)
//            {
//                //bool decoded = !(this->post[k] >= 0);
//                bool LLR_overall_test = (this->LLR_overall[k] < 0) ? true : false;


//                if (this->decoded_test[k] == LLR_overall_test)
//                {
//                    l++;
//                }
//                else
//                {
//                    this->decoded_test[k] = LLR_overall_test;
//                }
//            }

//            sameCount = ((l == this->N) ? sameCount + 1 : 0);

//            if (sameCount == 5)
//            {
//                std::cout << "m=" << this->H.get_n_cols() << " STOP AT ITER " << ite << std::endl;
//                break;
//            }







    }
    if (ite == this->n_ite)
        this->_compute_post(Y_N, this->msg_chk_to_var[frame_id], this->post);



    this->up_rule.end_decoding();


//    write_vector("_decode_msg_var_to_chk_003", this->msg_var_to_chk[frame_id]);
//    write_vector("_decode_msg_chk_to_var_003", this->msg_chk_to_var[frame_id]);
}



template <typename B, typename R>
void Decoder_LDPCA_BP_flooding_SPA<B,R>
::initialize_var_to_chk(const R *Y_N, const std::vector<R> &msg_chk_to_var, std::vector<R> &msg_var_to_chk)
{
    auto *msg_chk_to_var_ptr = msg_chk_to_var.data();
    auto *msg_var_to_chk_ptr = msg_var_to_chk.data();

    const auto n_var_nodes = (int)this->H.get_n_rows();;
    for (auto v = 0; v < n_var_nodes; v++)
    {
        const auto var_degree = (int)this->H.get_row_to_cols()[v].size();

        auto sum_msg_chk_to_var = (R)0;
        for (auto c = 0; c < var_degree; c++)
            sum_msg_chk_to_var += msg_chk_to_var_ptr[c];

        const auto tmp = Y_N[v] + sum_msg_chk_to_var;

        LLR_overall[v] = tmp;

        for (auto c = 0; c < var_degree; c++)
            msg_var_to_chk_ptr[c] = tmp - msg_chk_to_var_ptr[c];

        msg_chk_to_var_ptr += var_degree;
        msg_var_to_chk_ptr += var_degree;
    }
}




template <typename B, typename R>
void Decoder_LDPCA_BP_flooding_SPA<B,R>
::_decode_single_ite(const std::vector<R> &msg_var_to_chk, std::vector<R> &msg_chk_to_var)
{
//        std::cout << "\n\_decode_single_ite\n================================================================================================================\n===================================================================================================\n\n";

        //getchar();

//    this->write_vector("_decode_single_ite_msg_chk_to_var_001", msg_chk_to_var);
//    this->write_vector("_decode_single_ite_msg_var_to_chk_001", msg_var_to_chk);


	const auto n_branches = (int)this->H.get_n_connections();
	auto transpose_ptr = this->transpose.data();

	mipp::Reg<R> r_tmp;
	const mipp::Reg<R> r_1 = (R)1.0;
	const auto vec_loop_size = (n_branches / mipp::N<R>()) * mipp::N<R>();
	for (auto b = 0; b < vec_loop_size; b += mipp::N<R>())
	{
		r_tmp = &msg_var_to_chk[b];
		// tanh(1/2 * t) = (exp(t) - 1) / (exp(t) + 1)
		r_tmp = mipp::exp(r_tmp);
		r_tmp = (r_tmp - r_1) / (r_tmp + r_1);
		r_tmp.storeu(&msg_chk_to_var[b]);
	}

	// tail loop to compute the remaining elements
	for (auto b = vec_loop_size; b < n_branches; b++)
        msg_chk_to_var[b] = (R) std::tanh((R)0.5 * msg_var_to_chk[b]);


//    this->write_vector("_decode_single_ite_msg_chk_to_var_002", msg_chk_to_var);
//    this->write_vector("_decode_single_ite_msg_var_to_chk_002", msg_var_to_chk);


    double prod_check = 0;
    double val_check = 0;

	// flooding scheduling
	const auto n_chk_nodes = (int)this->H.get_n_cols();
	for (auto c = 0; c < n_chk_nodes; c++)
	{

//        std::cout << "\n========================\nc="<<c << std::endl;;

		const auto chk_degree = (int)this->H.get_col_to_rows()[c].size();


		//LDPCA EDIT HERE WITH 1 - 2*s
//        auto prod = (R)(1.0 - 2.0*_syndrome[c]);

//        std::cout<<"read Syndrome at "<< c;


        auto prod = (R)( (m_syndrome->at(c) ) ? -1.0 : 1.0);
        //auto prod = (R)(_syndrome[c]);

//        std::cout<<" val = "<< prod << std::endl;

//        std::cout<< m_syndrome.size() << std::endl;

//        for(int i = 0; i < m_syndrome.size() ; i++)
//        {
//            std::cout<< m_syndrome.at(i) << ", ";
//        }

//        std::cout<<std::endl;

		for (auto v = 0; v < chk_degree; v++)
		{
			this->values[v] = msg_chk_to_var[transpose_ptr[v]];
            //std::cout<<this->values[v]<<", ";
			prod *= this->values[v];
		}

        //getchar();

//        if(std::abs((double)prod) != prod_check)
//        {
//            std::cout << "c=" << c << ": prod new value=" << prod << std::endl;
//            prod_check = std::abs((double)prod);
//            //getchar();
//        }


               // std::cout << "***************************************************************" << std::endl;

		for (auto v = 0; v < chk_degree; v++)
		{
			auto val = prod / this->values[v];
			val = (std::abs(val) < (R)1.0) ? val : ((R)1.0 - std::numeric_limits<R>::epsilon()) * (val > 0 ? 1 : -1);
			msg_chk_to_var[transpose_ptr[v]] = (R)val;

//            if(std::abs((double)val) != val_check)
//            {
//                std::cout << "c=" << c << ": val new value=" << val << " =" << prod << "/" << this->values[v] << std::endl;
//                val_check = std::abs((double)val);
//               // getchar();
//            }
		}

		transpose_ptr += chk_degree;
	}

	for (auto b = 0; b < vec_loop_size; b += mipp::N<R>())
	{
		r_tmp = &msg_chk_to_var[b];
		// 2 * atanh(t) = log((1 + t) / (1 - t)
		r_tmp = mipp::log((r_1 + r_tmp) / (r_1 - r_tmp));
		r_tmp.storeu(&msg_chk_to_var[b]);

        //std::cout<<msg_chk_to_var[b]<<", ";

	}
	// tail loop to compute the remaining elements
	for (auto b = vec_loop_size; b < n_branches; b++)
    {
        msg_chk_to_var[b] = (R)2.0 * (R)std::atanh(msg_chk_to_var[b]);
        //std::cout<<msg_chk_to_var[b]<<", ";

    }

//        std::cout << "\n\FIN DE _decode_single_ite\n================================================================================================================\n===================================================================================================\n\n";

//    this->write_vector("_decode_single_ite_msg_chk_to_var_003", msg_chk_to_var);
//    this->write_vector("_decode_single_ite_msg_var_to_chk_003", msg_var_to_chk);
}
}
}
