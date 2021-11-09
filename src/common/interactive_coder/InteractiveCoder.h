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
    using IMatrixProvider_sp = std::shared_ptr<IMatrixProvider<Matrix>>;
    using IDecoder_sp = std::shared_ptr<IDecoder<Matrix>>;
    using ISyndromeManager_sp = std::shared_ptr<ISyndromeManager>;
    using LLRProvider_sp = std::shared_ptr<LLRProvider>;

public:

    struct encode_parameters
    {
        int start_code;
        int stop_code;
        int step;
    };

    InteractiveCoder(IDecoder<Matrix> *dec, IMatrixProvider<Matrix> *mp, ISyndromeManager *syndromeMgr, LLRProvider *llrProvider):
       m_pdecoder(dec), m_psyndromeMgr(syndromeMgr), m_pllrProvider(llrProvider),
       m_matrix_provider(mp)
    {
        //TODO INIT
        m_decodage_syndrome = std::unique_ptr<Syndrome>(new Syndrome(syndromeMgr->get_n()));
    };


    InteractiveCoder(const IDecoder_sp &dec, const IMatrixProvider_sp &mp, const ISyndromeManager_sp &syndromeMgr, const LLRProvider_sp &llrProvider):
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

    int encode(uint32_t &io_numcode, Syndrome &o_stored_syndrome, const std::vector<CodedVal> &source, uint32_t n_ite, const std::vector<CodedVal> &mask = std::vector<CodedVal>{});

    int encode(uint32_t &io_numcode, Syndrome &o_stored_syndrome, const std::vector<CodedVal> &source, uint32_t numcodeMax, uint32_t step, uint32_t n_ite, const std::vector<CodedVal> &mask = std::vector<CodedVal>{});

    int encodeLoop(uint32_t &io_numcode, Syndrome &o_stored_syndrome, const std::vector<CodedVal> &source,  const std::vector<encode_parameters> &params, uint32_t n_ite, const std::vector<CodedVal> &mask = std::vector<CodedVal>{});

    void decode(std::vector<CodedVal>& o_decoded, uint32_t numcode,  uint32_t n_ite, const Syndrome& syndrome);

    bool checkDecodingSyndrome(const Syndrome &syndrome, const std::vector<CodedVal> &source, int numcode) const;


    IMatrixProvider_sp matrixProvider() const {return  this->m_matrix_provider;}
    IDecoder_sp decoder() const {return  this->m_pdecoder;}
    ISyndromeManager_sp syndromeMgr() const {return  this->m_psyndromeMgr;}
    LLRProvider_sp llrProvider() const {return  this->m_pllrProvider;}


protected:


    IMatrixProvider_sp m_matrix_provider;
    IDecoder_sp m_pdecoder;
    ISyndromeManager_sp m_psyndromeMgr;
    LLRProvider_sp m_pllrProvider;

    std::unique_ptr<Syndrome> m_decodage_syndrome;
};
}

#include "InteractiveCoder.hpp"

#endif // INTERACTIVECODER_H
