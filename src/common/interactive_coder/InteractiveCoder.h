#ifndef INTERACTIVECODER_H
#define INTERACTIVECODER_H

#include <memory>

#include "common.h"
#include "interface/IMatrixProvider.h"
#include "interface/IDecoder.h"
#include "interface/ISyndrome.h"
#include "LLRProvider.h"

namespace icov {

template <class Matrix>
class InteractiveCoder
{
public:

    struct encode_parameters
    {
        int start_code;
        int stop_code;
        int step;
    };

    InteractiveCoder(IDecoder<Matrix>* dec, IMatrixProvider<Matrix> *mp, ISyndromeManager* syndromeMgr, LLRProvider* llrProvider):
       m_pdecoder(dec), m_psyndromeMgr(syndromeMgr), m_pllrProvider(llrProvider),
       m_matrix_provider(mp)
    {
        m_decodage_syndrome = std::unique_ptr<Syndrome>(new Syndrome(syndromeMgr->get_n()));
    };

    virtual ~InteractiveCoder() = default;

    void buildMatrices(int m_start, int m_end, bool inverse = false);


    virtual const Matrix& getMatrix(int numcode) const;

    bool testConvergence(const std::vector<CodedVal> &decoded, const std::vector<CodedVal> &source);

    virtual bool testConvergence(const std::vector<CodedVal> &decoded, const std::vector<CodedVal> &source, const std::vector<CodedVal> &mask);

    virtual int errorCount(const std::vector<CodedVal>& decoded, const std::vector<CodedVal>& source, const std::vector<CodedVal> &mask);

    int encode(uint32_t &io_numcode, Syndrome &o_stored_syndrome, const std::vector<CodedVal> &source, uint32_t numcodeMax, uint32_t step, uint32_t n_ite, const std::vector<CodedVal> &mask = std::vector<CodedVal>{});

    int encodeLoop(uint32_t &io_numcode, Syndrome &o_stored_syndrome, const std::vector<CodedVal> &source,  const std::vector<encode_parameters> &params, uint32_t n_ite, const std::vector<CodedVal> &mask = std::vector<CodedVal>{});

    void decode(std::vector<CodedVal>& o_decoded, uint32_t numcode,  uint32_t n_ite, const Syndrome& syndrome);

    bool checkDecodingSyndrome(const Syndrome &syndrome, const std::vector<CodedVal> &source, int numcode) const;




protected:

    inline IDecoder<Matrix>* decoder() const {return  this->m_pdecoder.get();}
    inline ISyndromeManager* syndromeMgr() const {return  this->m_psyndromeMgr.get();}
    inline LLRProvider* llrProvider() const {return  this->m_pllrProvider.get();}

    std::shared_ptr<IMatrixProvider<Matrix>> m_matrix_provider;
    std::unique_ptr<Syndrome> m_decodage_syndrome;
    std::unique_ptr<IDecoder<Matrix>> m_pdecoder;
    std::unique_ptr<ISyndromeManager> m_psyndromeMgr;
    std::unique_ptr<LLRProvider> m_pllrProvider;
};


}

#endif // INTERACTIVECODER_H
