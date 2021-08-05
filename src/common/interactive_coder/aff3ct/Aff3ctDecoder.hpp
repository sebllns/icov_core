#include "Aff3ctDecoder.h"


#include "Decoder_LDPCA_BP_flooding_SPA.hpp"



icov::Aff3ctDecoder::Aff3ctDecoder() : IDecoder()
{

}

void icov::Aff3ctDecoder::decode(std::vector<CodedVal> &o_decoded, int n, int numcode, int n_ite, const std::vector<SIVal> &llr, Syndrome &syndrome, const AffMatrix &matrix)
{

    int k = n;

    std::vector<uint32_t> info_bits_pos; //empty vector

//        std::cout<< numcode << std::endl;

     auto affdec = module::Decoder_LDPCA_BP_flooding_SPA<>(k, n, n_ite, matrix, info_bits_pos, &syndrome, false, 1, 1);


//     std::cout<<"setSyndrome\n";
//     std::cout<< syndrome.size() << std::endl;

//     for(int i = 0; i < syndrome.size() ; i++)
//     {
//         std::cout<< syndrome.at(i) << ", ";
//     }

//      std::cout<< std::endl;

     //affdec.setSyndrome(syndrome);
     affdec.decode_siho(llr, o_decoded);

     //std::cout<<"end decode\n";

}

