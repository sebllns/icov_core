#include "InteractiveCoder.h"
#include "common.h"

using namespace icov;

template<class Matrix>
bool InteractiveCoder<Matrix>::testConvergence(const BitVec &decoded, const BitVec &source)
{
    if(decoded.size() != source.size()) return false;

    for (int i = 0; i < decoded.size(); i++)
    {
        if(decoded[i] != source[i]) return false;
    }

    return true;
}

template<class Matrix>
int InteractiveCoder<Matrix>::errorCount(const BitVec &decoded, const BitVec &source)
{
    float numerrors = 0;
    for (int i = 0; i < decoded.size(); i++)
    {
        numerrors += std::abs(decoded[i] - source[i]);
    }

    return (int)numerrors;
}

template<class Matrix>
int InteractiveCoder<Matrix>::encode(uint32_t &i,  Syndrome &o_stored_syndrome, const BitVec &source)
{
    o_stored_syndrome.resize(0);

    BitVec decoded(m_psyndromeMgr->get_n());

    bool convergence = false;
    while(i < m_matrix_provider->getNumCodesCount() && !convergence)
    {
        uint32_t io_numcode = m_matrix_provider->getNumcode(i);

//        std::cout << ">> m=" << m_psyndromeMgr->get_m(io_numcode) << std::endl;

        m_psyndromeMgr->compute_stored(o_stored_syndrome, io_numcode, *m_pllrProvider.get());

        this->decode(decoded, i, o_stored_syndrome);

        convergence = testConvergence(decoded, source);
        if(!convergence) ++i;
    }

    if(convergence) return 0;
    return errorCount(decoded, source);
}

template<class Matrix>
int InteractiveCoder<Matrix>::encode(uint32_t &io_numcode,  Syndrome &o_stored_syndrome, const BitVec &source, uint32_t numcodeMax, uint32_t step)
{
    int n = source.size();

    BitVec decoded(m_psyndromeMgr->get_n());

    uint32_t next_test_code = io_numcode;

    for(io_numcode = 0 ; io_numcode < numcodeMax ; io_numcode++)
    {
        //std::cout << io_numcode << std::endl;

        m_psyndromeMgr->compute_stored(o_stored_syndrome, io_numcode, *m_pllrProvider.get());

        if(io_numcode == next_test_code)
        {
            this->decode(decoded, io_numcode, o_stored_syndrome);

            if(testConvergence(decoded, source))
            {
                return 0;
            }

            next_test_code = std::min(next_test_code+step, numcodeMax-1);
        }
    }

    io_numcode = numcodeMax - 1;
    return errorCount(decoded, source);
}

template<class Matrix>
void InteractiveCoder<Matrix>::decode(BitVec &o_decoded, uint32_t i, const Syndrome &stored_syndrome)
{
    m_psyndromeMgr->compute_deacc(*m_decodage_syndrome.get(), stored_syndrome, m_matrix_provider->getNumcode(i), *m_pllrProvider.get());
    m_pdecoder->decode(o_decoded, m_psyndromeMgr->get_n(), i, m_pllrProvider->llr(), *m_decodage_syndrome.get(), m_matrix_provider->getMatrix(i));
}
