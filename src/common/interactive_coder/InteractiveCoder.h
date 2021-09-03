#ifndef INTERACTIVECODER_H
#define INTERACTIVECODER_H

#include <memory>

#include "common.h"
#include "interface/IMatrixBuilder.h"
#include "interface/IDecoder.h"
#include "interface/ISyndrome.h"
#include "LLRProvider.h"

namespace icov {

template <class Matrix>
class InteractiveCoder
{
public:
    InteractiveCoder(IDecoder<Matrix>* dec, ISyndromeManager* syndromeMgr, LLRProvider* llrProvider):
       m_pdecoder(dec), m_psyndromeMgr(syndromeMgr), m_pllrProvider(llrProvider)
    {
        m_decodage_syndrome = std::unique_ptr<Syndrome>(new Syndrome(syndromeMgr->get_n()));
    };

    virtual ~InteractiveCoder() = default;

    void buildMatrices(IMatrixBuilder<Matrix>& mb, int m_start, int m_end);


    virtual const Matrix& getMatrix(int numcode) const;


    virtual bool testConvergence(const std::vector<CodedVal>& decoded, const std::vector<CodedVal>& source);

    virtual int errorCount(const std::vector<CodedVal>& decoded, const std::vector<CodedVal>& source);

    int encode(uint32_t &o_numcode, Syndrome &o_stored_syndrome, const std::vector<CodedVal> &source, uint32_t numcodeMax, uint32_t step, uint32_t n_ite);

    void decode(std::vector<CodedVal>& o_decoded, uint32_t numcode,  uint32_t n_ite, const Syndrome& syndrome);


protected:

    inline IDecoder<Matrix>* decoder() const {return  this->m_pdecoder.get();}
    inline ISyndromeManager* syndromeMgr() const {return  this->m_psyndromeMgr.get();}
    inline LLRProvider* llrProvider() const {return  this->m_pllrProvider.get();}

    std::vector<std::unique_ptr<Matrix>> m_matrices;
    std::unique_ptr<Syndrome> m_decodage_syndrome;
    std::unique_ptr<IDecoder<Matrix>> m_pdecoder;
    std::unique_ptr<ISyndromeManager> m_psyndromeMgr;
    std::unique_ptr<LLRProvider> m_pllrProvider;
};


}

#endif // INTERACTIVECODER_H
