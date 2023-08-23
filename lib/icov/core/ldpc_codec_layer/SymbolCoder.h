#pragma once

#include "ldpc_codec_layer/BitSymbolProvider.h"
#include "ldpc_codec_layer/ZeroErrorCoder.h"
#include "ldpc_codec_layer/interface/IInteractiveCoder.h"
namespace icov {
namespace ldpc_codec_layer {

inline void init_LLR_bitplane(LLRProvider &llr, const BitSymbolProvider &bsp, int bitplane_idx, const BitVec &signs,
                              const SymboleVec &predec)
{

    // init the LLR values
    if (bitplane_idx == 0)
    {
        // get LLR for first bitplane level i.e. sign
        llr.initLLR_s(bsp.distribution(), bsp.si_symboles());
    }
    else
    {
        // get LLR for bitplane after the sign
        llr.initLLR_b(bitplane_idx - 1, bsp.distribution(), bsp.si_symboles(), signs, predec);
    }
}

class IBitplaneEncoder
{
};

template <class Matrix> class BitplaneEncoder : IBitplaneEncoder
{

  public:
    struct t_params
    {
        bool CheckBitplaneEncoding = false;
    };

    BitplaneEncoder(const BitSymbolProvider &bsp, ZeroErrorEncoder<Matrix> &encoder, const t_params &params)
        : bsp(bsp), m_encoder(encoder), params(params)
    {
    }

    NumcodeIdx encode_bitplane(int bitplane_idx, NumcodeIdx &numcodeIdx, Syndrome &stored_synd, SymboleVec &predec)
    {
        auto m_coder = this;
        auto syndromeMgr = m_coder->syndromeMgr();
        const auto &src_symbols = bsp.src_symboles();
        const auto &src_bits = bsp.src_bits();
        const auto &bitplane_bits = src_bits.at(bitplane_idx);

        const auto &sign_bits = src_bits[0];

        auto m_pllrProvider = m_encoder.llrProvider();

        int n = syndromeMgr->get_n();
        int lx = bsp.bitplane_count();
        int shift = (lx - bitplane_idx - 1);

        //    syndrome accumulation
        syndromeMgr->init_accumulation(bitplane_bits);

        init_LLR_bitplane(*m_pllrProvider, bsp, bitplane_idx, sign_bits, predec);

        const NumcodeIdx no_coding_flag = 0;
        const NumcodeIdx coding_flag = 1;
        const NumcodeIdx coding_offset = coding_flag + 1;

        // TO CODE OR NOT TO CODE ???
        numcodeIdx = std::equal(m_pllrProvider->llr().begin(), m_pllrProvider->llr().end(), bitplane_bits.begin(),
                                [](LLRVal llr, BitVal srcBit) {
                                    return (llr > 0 && srcBit == bit_t::zero) || (llr < 0 && srcBit == bit_t::one);
                                })
                         ? no_coding_flag
                         : coding_flag;

        if (numcodeIdx == coding_flag)
        {
            try
            {
                int numerrors = this->encode(numcodeIdx, stored_synd, bitplane_bits);
                icov::assert::check(numerrors == 0, "No convergence for bitplane " + std::to_string(bitplane_idx));

                if (params.CheckBitplaneEncoding)
                {
                    ICOV_TRACE << "CHECK SYNDROME BP " << bitplane_idx;
                    BitVec decoded;
                    m_coder->decode(decoded, numcodeIdx, stored_synd);
                    icov::assert::check(decoded == src_bits[bitplane_idx],
                                        "INTERNAL Decoding BP check failed " + std::to_string(bitplane_idx));
                }

                numcodeIdx += coding_offset;
            }
            catch (const std::exception &e)
            {
                numcodeIdx = 1;
                stored_synd.fillFrom(bitplane_bits);
                ICOV_WARNING << "Bitplane " << bitplane_idx;
                ICOV_WARNING << "Encoding bitplane as source as we catched exception in "
                                "bitplane encoding: "
                             << e.what();
                ICOV_ERROR << boost::current_exception_diagnostic_information();
            }
            catch (...)
            {
                numcodeIdx = 1;
                stored_synd.fillFrom(bitplane_bits);
                ICOV_WARNING << "Bitplane " << bitplane_idx;
                ICOV_WARNING << "Encoding bitplane as source as we catched unknown exception in "
                                "bitplane encoding";
                ICOV_ERROR << boost::current_exception_diagnostic_information();
            }
        }
        else
        {
            stored_synd.resize(0);
            ICOV_TRACE << "ENCODE: NO CODING";
        }

        predec.resize(n);

        std::transform(src_symbols.cbegin(), src_symbols.cend(), predec.begin(), [&](int srcSymbol) {
            if (bitplane_idx == 0)
                return 0;
            if (shift == 0)
                return srcSymbol;
            return std::abs(srcSymbol) >> shift;
        });
    }

  private:
    const BitSymbolProvider &bsp;
    ZeroErrorEncoder<Matrix> &m_encoder;
    t_params params;
};

class IBitplaneVecEncoder
{
};

template <class Matrix> class BitplaneVecEncoder : IBitplaneVecEncoder
{

  public:
    BitplaneVecEncoder(const BitSymbolProvider &bsp, ZeroErrorEncoder<Matrix> &encoder, SyndromesVec &stored_synd,
                       SymboleVec &predec)
        : m_bsp(bsp), m_encoder(encoder), stored_synd(stored_synd), predec(predec)
    {
        int n = m_encoder.get_n();
        size_t lx = bsp.bitplane_count();

        if (stored_synd.size() != lx)
        {
            stored_synd = SyndromesVec(lx, SyndromesVec::value_type(n));
        }

        predec.resize(n);
    }

    NumcodeIdxVec encode_bitplanes()
    {
        int n = m_encoder.get_n();
        size_t lx = m_bsp.bitplane_count();
        NumcodeIdxVec numcodeIdx(lx, 0);
        std::fill(predec.begin(), predec.end(), 0);

        for (size_t i = 0; i < lx; i++)
        {
            ICOV_TRACE << "ENCODE BP " << i;
            stored_synd[i].resize(n);
            stored_synd[i].fill(0);
            encode_bitplane(m_bsp, i, numcodeIdx[i], stored_synd[i], predec);
        }

        ICOV_ASSERT_MSG(predec == bsp.src_symboles(), "Symbole values are not recovered from encoding process");
    }

    NumcodeIdx encode_bitplane(int bitplane_idx);

  private:
    const BitSymbolProvider &m_bsp;
    ZeroErrorEncoder<Matrix> &m_encoder;
    SyndromesVec &stored_synd;
    SymboleVec &predec;
};

class ISymbolEncoder
{
};

template <class Matrix> class SymbolEncoder : ISymbolEncoder
{
};

class ISymbolDecoder
{
};

class SymbolCoder
{
  public:
    SymbolCoder(IInteractiveCoder &bitcoder);

    const IInteractiveCoder &bitcoder() const;

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

    void init_LLR_bitplane(const BitSymbolProvider &bsp, int bitplane_idx, const BitVec &signs,
                           const SymboleVec &predec);

    int get_n() const;

  private:
    IInteractiveCoder &m_bitcoder;
    BitVec m_decoded_bits;
    SymboleVec m_predec;
};
} // namespace ldpc_codec_layer
} // namespace icov
