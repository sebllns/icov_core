#ifndef AFF3CTDECODER_HPP
#define AFF3CTDECODER_HPP

#include "Aff3ctDecoder.h"



//#include <chrono>


icov::Aff3ctDecoder::Aff3ctDecoder(int n_dec) : IDecoder()
  //,    m_decvector(n_dec)
{

}

void icov::Aff3ctDecoder::decode(std::vector<CodedVal> &o_decoded, int n, int numcode, int n_ite, const std::vector<SIVal> &llr, Syndrome &syndrome, const AffMatrix &matrix)
{
    int d = std::min(numcode, (int)m_decvector.size()-1);
    m_decvector[d]-> reset();
    m_decvector[d]-> setSyndrome(&syndrome);
    m_decvector[d]->decode_siho(llr, o_decoded);
}


void icov::Aff3ctDecoder::decode_doping(std::vector<CodedVal> &o_decoded, std::vector<size_t> &o_doping_idx, int n, int numcode, int n_ite, const std::vector<SIVal> &llr, Syndrome &syndrome, const AffMatrix &matrix)
{
     auto affdec = AffLDPCA(n, n_ite, matrix);
     affdec.setSyndrome(&syndrome);
     affdec.decode_siho(llr, o_decoded);
     affdec.get_llr_order(o_doping_idx);
}

void icov::Aff3ctDecoder::init_decoder(int numcode, int n, int n_ite, const AffMatrix &matrix)
{
    m_decvector.push_back(std::unique_ptr<AffLDPCA>(new AffLDPCA(n, n_ite, matrix)));
}

#endif // AFF3CTDECODER_HPP
