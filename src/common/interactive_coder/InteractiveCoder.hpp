#include "InteractiveCoder.h"
#include "common_types.h"

using namespace icov;


// m = m_12 -> ordonner tous les LLR
template <class Matrix, class SyndromeTools>
void InteractiveCoder<Matrix, SyndromeTools>::buildMatrices(IMatrixBuilder<Matrix>& mb, int m_start, int m_end)
{
    for(int m = m_start ; m < m_end ; m++)
    {
        this->m_matrices.push_back(std::unique_ptr<Matrix>(mb.buildMatrix(m)));
    }
}

template<class Matrix, class SyndromeTools>
const Matrix& InteractiveCoder<Matrix, SyndromeTools>::getMatrix(int numcode) const
{
    return *(m_matrices[std::min(numcode, (int)m_matrices.size()-1)]);
}

template<class Matrix, class SyndromeTools>
bool InteractiveCoder<Matrix, SyndromeTools>::testConvergence(const std::vector<CodedVal> &decoded, const std::vector<CodedVal> &source)
{
    return  decoded == source;
}

template<class Matrix, class SyndromeTools>
int InteractiveCoder<Matrix, SyndromeTools>::errorCount(const std::vector<CodedVal> &decoded, const std::vector<CodedVal> &source)
{
    float numerrors = 0;
    for (int i = 0; i < decoded.size(); i++)
    {
        numerrors += std::abs(decoded[i] - source[i]);
    }

    return (int)numerrors;
}

template<class Matrix, class SyndromeTools>
int InteractiveCoder<Matrix, SyndromeTools>::encode(uint32_t &io_numcode,  Syndrome &o_stored_syndrome, const std::vector<CodedVal> &source, uint32_t numcodeMax, uint32_t n_ite, const ISyndromeEncodage<SyndromeTools>& synd_enc, const ISyndromeDecodage<SyndromeTools>& synd_dec)
{
    int n = source.size();

    std::vector<CodedVal> decoded(tools()->get_n());

    for( ; io_numcode < numcodeMax ; io_numcode++)
    {
        synd_enc.compute(o_stored_syndrome, io_numcode, *tools(), *llrProvider());
         this->decode(decoded, io_numcode, n_ite, o_stored_syndrome, synd_dec);

        if(testConvergence(decoded, source))
        {
            return 0;
        }
    }

    std::cout << io_numcode << std::endl;
    io_numcode = numcodeMax - 1;
    return errorCount(decoded, source);
}

template<class Matrix, class SyndromeTools>
void InteractiveCoder<Matrix, SyndromeTools>::decode(std::vector<CodedVal> &o_decoded, uint32_t numcode, uint32_t n_ite, const Syndrome &stored_syndrome, const ISyndromeDecodage<SyndromeTools>& synd_dec)
{
    synd_dec.compute(*m_decodage_syndrome.get(), stored_syndrome, numcode, *tools(), *llrProvider());
    decoder()->decode(o_decoded, tools()->get_n(), numcode, n_ite, m_pllrProvider->llr(), *m_decodage_syndrome.get(), getMatrix(numcode));
}
