#pragma once

#include "BitSymbolProvider.h"
#include "LLRProvider.h"
#include "interface/ILDPC_Decoder.h"
#include "interface/IMatrixProvider.h"
#include "interface/ISyndrome.h"
#include <memory>

namespace icov {
namespace ldpc_codec_layer {

using Rate = float;
using NumcodeIdx = uint32_t;

class IZeroErrorDecoder
{
  public:
    enum SyndromeType
    {
        SyndromeType_NO_CODING = 0,
        SyndromeType_SOURCE = 1,
        SyndromeType_NORMAL = 2,
        SyndromeType_COUNT
    };

    static struct t_params
    {
        bool CheckBitplaneEncoding;
    } params;
    virtual ~IZeroErrorDecoder() = default;

    virtual size_t get_n() const = 0;
    virtual int get_m(const NumcodeIdx &numcodeIdx) const = 0;
    virtual int get_m_min() const = 0;
    virtual int get_m_max() const = 0;
    virtual Rate get_r(const NumcodeIdx &numcodeIdx) const = 0;
    virtual Rate get_r(int m, int n) const = 0;
    virtual bool testConvergence(const BitVec &decoded, const BitVec &source) const = 0;
    virtual int errorCount(const BitVec &decoded, const BitVec &source) const = 0;

    virtual void decode(BitVec &o_decoded, NumcodeIdx numcodeIdx, const Syndrome &syndrome) = 0;
};

template <class Matrix> class ZeroErrorDecoder : public IZeroErrorDecoder
{
    using IMatrixProvider_sp = std::shared_ptr<IMatrixProvider<Matrix>>;
    using IDecoder_sp = std::shared_ptr<ILDPC_Decoder<Matrix>>;
    using ISyndromeManager_sp = std::shared_ptr<ISyndromeManager>;
    using LLRProvider_sp = std::shared_ptr<LLRProvider>;

  public:
    ZeroErrorDecoder(const IDecoder_sp &dec, const IMatrixProvider_sp &mp, const ISyndromeManager_sp &syndromeMgr,
                     const LLRProvider_sp &llrProvider)
        : m_pdecoder(dec), m_psyndromeMgr(syndromeMgr), m_pllrProvider(llrProvider), m_matrix_provider(mp),
          m_decodage_syndrome(std::unique_ptr<Syndrome>(new Syndrome(syndromeMgr->get_n())))
    {
    }

    ~ZeroErrorDecoder() = default;

    size_t get_n() const override
    {
        return m_psyndromeMgr->get_n();
    }

    int get_m(const NumcodeIdx &numcodeIdx) const override
    {
        return m_psyndromeMgr->get_m(m_matrix_provider->getNumcode(numcodeIdx));
    }

    int get_m_min() const override
    {
        return m_psyndromeMgr->get_m(m_matrix_provider->getNumcode(0));
    }

    int get_m_max() const override
    {
        return m_psyndromeMgr->get_m(m_matrix_provider->getNumcode(m_matrix_provider->getNumCodesCount() - 1));
    }

    Rate get_r(const NumcodeIdx &numcodeIdx) const override
    {
        return get_r(get_m(numcodeIdx), get_n());
    }

    Rate get_r(int m, int n) const override
    {
        return static_cast<Rate>(m) / static_cast<Rate>(n);
    }

    bool testConvergence(const BitVec &decoded, const BitVec &source) const override
    {
        if (decoded.size() != source.size())
            return false;

        for (size_t i = 0; i < decoded.size(); i++)
        {
            if (decoded[i] != source[i])
                return false;
        }

        return true;
    }

    int errorCount(const BitVec &decoded, const BitVec &source) const override
    {
        float numerrors = 0;
        for (size_t i = 0; i < decoded.size(); i++)
        {
            numerrors += std::abs(decoded[i] - source[i]);
        }
        return (int)numerrors;
    }

    void decode(BitVec &o_decoded, NumcodeIdx numcodeIdx, const Syndrome &syndrome) override
    {
        m_psyndromeMgr->compute_deacc(*m_decodage_syndrome.get(), syndrome, m_matrix_provider->getNumcode(numcodeIdx),
                                      *m_pllrProvider.get());
        m_pdecoder->decode(o_decoded, m_psyndromeMgr->get_n(), numcodeIdx, m_pllrProvider->llr(),
                           *m_decodage_syndrome.get(), m_matrix_provider->getMatrix(numcodeIdx));
    }

    IMatrixProvider_sp matrixProvider() const
    {
        return this->m_matrix_provider;
    }
    IDecoder_sp decoder() const
    {
        return this->m_pdecoder;
    }
    ISyndromeManager_sp syndromeMgr() const
    {
        return this->m_psyndromeMgr;
    }
    LLRProvider_sp llrProvider() const
    {
        return this->m_pllrProvider;
    }

  protected:
    IMatrixProvider_sp m_matrix_provider;
    IDecoder_sp m_pdecoder;
    ISyndromeManager_sp m_psyndromeMgr;
    LLRProvider_sp m_pllrProvider;

    std::unique_ptr<Syndrome> m_decodage_syndrome;
    //    BitVec m_decoded_bits;
    //    SymboleVec m_predec;
};

template <class Matrix> class ZeroErrorEncoder : public ZeroErrorDecoder<Matrix>
{
    int encode(NumcodeIdx &io_numcode, Syndrome &o_stored_syndrome, const BitVec &source);
    int encode(uint32_t &io_numcode, Syndrome &o_stored_syndrome, const BitVec &source, uint32_t numcodeMax,
               uint32_t step);
};

void encode_symboles(std::vector<NumcodeIdxVec> &rates, std::vector<SyndromesVec> &syndromes,
                     const std::vector<SIVec> &si, const QaryDistribution::quantization_params &p,
                     const SymboleVec &src);

void encode_bitplanes(const BitSymbolProvider &bsp, NumcodeIdxVec &rate, SyndromesVec &stored_synd);

void encode_bitplane(const BitSymbolProvider &bsp, int bitplane_idx, NumcodeIdx &numcodeIdx, Syndrome &stored_synd,
                     SymboleVec &predec);

///
/// \brief decode_symboles
/// \param io_bsp BitSymbolProvider that has been previously initialised with
/// correct parameters (Q, Zmin, Zmax, lx) \param si \param numcodes \param
/// stored_synd
///
void decode_symboles(BitSymbolProvider &io_bsp, const NumcodeIdxVec &numcodes, const SyndromesVec &stored_synd);

void decode_bitplane(BitVec &output, int bp_idx, BitSymbolProvider &bsp, const NumcodeIdx &numcode,
                     const Syndrome &stored_synd);

} // namespace ldpc_codec_layer
} // namespace icov

