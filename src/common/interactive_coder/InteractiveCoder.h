#ifndef INTERACTIVECODER_H
#define INTERACTIVECODER_H

#include <memory>

#include "common_types.h"
#include "interface/IMatrixBuilder.h"
#include "interface/IDecoder.h"
#include "interface/ISyndrome.h"
#include "LLRProvider.h"

namespace icov {

template <class Matrix, class SyndromeTools = DefaultSyndromeTools>
class InteractiveCoder
{
public:
    InteractiveCoder(IDecoder<Matrix>* dec, SyndromeTools* tools, LLRProvider* llrProvider):
       m_pdecoder(dec), m_ptools(tools), m_pllrProvider(llrProvider)
    {
        m_decodage_syndrome = std::unique_ptr<Syndrome>(new Syndrome(tools->get_n()));
    };

    virtual ~InteractiveCoder() = default;

    void buildMatrices(IMatrixBuilder<Matrix>& mb, int m_start, int m_end);


    virtual const Matrix& getMatrix(int numcode) const;


    virtual bool testConvergence(const std::vector<CodedVal>& decoded, const std::vector<CodedVal>& source);

    virtual int errorCount(const std::vector<CodedVal>& decoded, const std::vector<CodedVal>& source);

//    int get_m(int code) const {return tools()->get_m(code);}
//    int get_n() const {return tools()->get_n();}

    int encode(uint32_t &o_numcode, Syndrome &o_stored_syndrome, const std::vector<CodedVal> &source, const std::vector<SIVal> &side_info, uint32_t numcodeMax, uint32_t n_ite, const ISyndromeEncodage<SyndromeTools>& synd_enc, const ISyndromeDecodage<SyndromeTools>& synd_dec);

    void decode(std::vector<CodedVal>& o_decoded, uint32_t numcode,  uint32_t n_ite, const std::vector<SIVal>& side_info, const Syndrome& syndrome, const ISyndromeDecodage<SyndromeTools>& synd_dec);


protected:

    inline IDecoder<Matrix>* decoder() const {return  this->m_pdecoder.get();}
    inline SyndromeTools* tools() const {return  this->m_ptools.get();}
    inline LLRProvider* llrProvider() const {return  this->m_pllrProvider.get();}

    inline ISyndromeAccumulation<SyndromeTools>* synd_acc() const {return  this->m_psyndAcc.get();}

    std::vector<std::unique_ptr<Matrix>> m_matrices;
    std::unique_ptr<Syndrome> m_decodage_syndrome;
    std::unique_ptr<IDecoder<Matrix>> m_pdecoder;
    std::unique_ptr<ISyndromeAccumulation<SyndromeTools>> m_psyndAcc;
    std::unique_ptr<SyndromeTools> m_ptools;
    std::unique_ptr<LLRProvider> m_pllrProvider;
};


}

#endif // INTERACTIVECODER_H
