#include "InteractiveCoder.h"
#include "common.h"

using namespace icov;


// m = m_12 -> ordonner tous les LLR
template <class Matrix>
void InteractiveCoder<Matrix>::buildMatrices(int m_start, int m_end, bool inverse)
{
    if(inverse)
    {
        for(int m = m_end-1 ; m >= m_start ; m--)
        {
            m_matrix_provider->buildMatrix(m);
        }
    }
    else
    {
        for(int m = m_start ; m < m_end ; m++)
        {
            m_matrix_provider->buildMatrix(m);
        }
    }
}

template<class Matrix>
const Matrix& InteractiveCoder<Matrix>::getMatrix(int numcode) const
{
    return m_matrix_provider->getMatrix(numcode);
}

template<class Matrix>
bool InteractiveCoder<Matrix>::testConvergence(const std::vector<CodedVal> &decoded, const std::vector<CodedVal> &source)
{
    return this->testConvergence(decoded, source, std::vector<CodedVal>());
}

template<class Matrix>
bool InteractiveCoder<Matrix>::testConvergence(const std::vector<CodedVal> &decoded, const std::vector<CodedVal> &source, const std::vector<CodedVal> &mask)
{
    if(decoded.size() != source.size()) return false;

    bool use_mask = mask.size() == source.size();
    CodedVal mask_value = 1;
    for (int i = 0; i < decoded.size(); i++)
    {
        mask_value = use_mask ? mask[i] : 1;
        if(mask_value && decoded[i] != source[i]) return false;
    }

    return true;
}

template<class Matrix>
int InteractiveCoder<Matrix>::errorCount(const std::vector<CodedVal> &decoded, const std::vector<CodedVal> &source, const std::vector<CodedVal> &mask)
{
    float numerrors = 0;
    bool use_mask = mask.size() == source.size();
    CodedVal mask_value = 1;
    for (int i = 0; i < decoded.size(); i++)
    {
        mask_value = use_mask ? mask[i] : 1;
        numerrors += mask_value * std::abs(decoded[i] - source[i]);
    }

    return (int)numerrors;
}

template<class Matrix>
bool InteractiveCoder<Matrix>::checkDecodingSyndrome(const Syndrome &syndrome, const std::vector<CodedVal> &source, int numcode) const
{
    bool synd_test;

    if(syndrome.size() != m_psyndromeMgr->get_m(numcode))
    {
        std::cout << "error" << std::endl;
        return false;
    }

    //not working for m > n/2
    for(int i = 0 ; i < syndrome.size(); i++)
    {
        synd_test = 0;
        for(const auto& vn: getMatrix(numcode).get_rows_from_col(i))
        {
            synd_test ^= source[vn];
        }

        if(synd_test != syndrome.at(i))
        {
            std::cout << "error " << std::endl;
            return false;
        }
    }

    return true;
}

template<class Matrix>
int InteractiveCoder<Matrix>::encodeLoop(uint32_t &numcode, Syndrome &o_stored_syndrome, const std::vector<CodedVal> &source,  const std::vector<encode_parameters> &params, uint32_t n_ite, const std::vector<CodedVal> &mask)
{
    //syndrome accumulation
    m_psyndromeMgr->init_accumulation(source);

    int numerrors = -1;
    numcode = 0;
    for(auto &param: params)
    {
        numcode = std::max(numcode, (uint32_t) param.start_code);
        int end_code = param.stop_code;

        if(end_code <= numcode)
        {
            continue;
        }

        numerrors = this->encode(numcode, o_stored_syndrome, source, end_code, param.step, n_ite, mask);

        //encoding done (no error)
        if(numerrors == 0) break;
    }

    return numerrors;
}

template<class Matrix>
int InteractiveCoder<Matrix>::encode(uint32_t &i,  Syndrome &o_stored_syndrome, const std::vector<CodedVal> &source, uint32_t n_ite, const std::vector<CodedVal> &mask)
{
    std::vector<CodedVal> decoded(m_psyndromeMgr->get_n());



    bool convergence = false;
    while(i < m_matrix_provider->getNumCodesCount() && !convergence)
    {
        uint32_t io_numcode = m_matrix_provider->getNumcode(i);

        //std::cout << i << "->" << io_numcode << std::endl;

        m_psyndromeMgr->compute_stored(o_stored_syndrome, io_numcode, *m_pllrProvider.get());

        this->decode(decoded, i, n_ite, o_stored_syndrome);

        ++i;
        convergence = testConvergence(decoded, source, mask);
    }

    --i;
    if(convergence) return 0;
    return errorCount(decoded, source, mask);
}

template<class Matrix>
int InteractiveCoder<Matrix>::encode(uint32_t &io_numcode,  Syndrome &o_stored_syndrome, const std::vector<CodedVal> &source, uint32_t numcodeMax, uint32_t step, uint32_t n_ite, const std::vector<CodedVal> &mask)
{
    int n = source.size();

    std::vector<CodedVal> decoded(m_psyndromeMgr->get_n());

    uint32_t next_test_code = io_numcode;

    for(io_numcode = 0 ; io_numcode < numcodeMax ; io_numcode++)
    {
        //std::cout << io_numcode << std::endl;

        m_psyndromeMgr->compute_stored(o_stored_syndrome, io_numcode, *m_pllrProvider.get());

        if(io_numcode == next_test_code)
        {
            this->decode(decoded, io_numcode, n_ite, o_stored_syndrome);

            if(testConvergence(decoded, source, mask))
            {
                return 0;
            }

            next_test_code = std::min(next_test_code+step, numcodeMax-1);
        }
    }

    io_numcode = numcodeMax - 1;
    return errorCount(decoded, source, mask);
}

template<class Matrix>
void InteractiveCoder<Matrix>::decode(std::vector<CodedVal> &o_decoded, uint32_t i, uint32_t n_ite, const Syndrome &stored_syndrome)
{
    m_psyndromeMgr->compute_deacc(*m_decodage_syndrome.get(), stored_syndrome, m_matrix_provider->getNumcode(i), *m_pllrProvider.get());
    m_pdecoder->decode(o_decoded, m_psyndromeMgr->get_n(), i, n_ite, m_pllrProvider->llr(), *m_decodage_syndrome.get(), getMatrix(i));
}
