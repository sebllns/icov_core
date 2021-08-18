#ifndef AFF3CTDECODER_HPP
#define AFF3CTDECODER_HPP

#include "Aff3ctDecoder.h"


#include "Decoder_LDPCA_BP_flooding_SPA.hpp"

//#include <chrono>


icov::Aff3ctDecoder::Aff3ctDecoder() : IDecoder()
{

}

void icov::Aff3ctDecoder::decode(std::vector<CodedVal> &o_decoded, int n, int numcode, int n_ite, const std::vector<SIVal> &llr, Syndrome &syndrome, const AffMatrix &matrix)
{
    // auto start = std::chrono::steady_clock::now();
    int k = n;
    std::vector<uint32_t> info_bits_pos; //empty vector
     auto affdec = module::Decoder_LDPCA_BP_flooding_SPA<CodedVal, SIVal>(k, n, n_ite, matrix, info_bits_pos, &syndrome, false, 1, 1);
     affdec.decode_siho(llr, o_decoded);
   //  auto end = std::chrono::steady_clock::now();
}


void icov::Aff3ctDecoder::decode_doping(std::vector<CodedVal> &o_decoded, std::vector<size_t> &o_doping_idx, int n, int numcode, int n_ite, const std::vector<SIVal> &llr, Syndrome &syndrome, const AffMatrix &matrix)
{
    int k = n;
    std::vector<uint32_t> info_bits_pos; //empty vector
     auto affdec = module::Decoder_LDPCA_BP_flooding_SPA<CodedVal, SIVal>(k, n, n_ite, matrix, info_bits_pos, &syndrome, false, 1, 1);
     affdec.decode_siho(llr, o_decoded);
     affdec.get_llr_order(o_doping_idx);
}

#endif // AFF3CTDECODER_HPP
