#include "InteractiveCoder.h"
#include "common.h"

//#define TEST_SYNDROME

using namespace icov;


// m = m_12 -> ordonner tous les LLR
template <class Matrix>
void InteractiveCoder<Matrix>::buildMatrices(IMatrixBuilder<Matrix>& mb, int m_start, int m_end)
{
    for(int m = m_start ; m < m_end ; m++)
    {
        this->m_matrices.push_back(std::unique_ptr<Matrix>(mb.buildMatrix(m)));
    }
}

template<class Matrix>
const Matrix& InteractiveCoder<Matrix>::getMatrix(int numcode) const
{
    return *(m_matrices[std::min(numcode, (int)m_matrices.size()-1)]);
}

template<class Matrix>
bool InteractiveCoder<Matrix>::testConvergence(const std::vector<CodedVal> &decoded, const std::vector<CodedVal> &source)
{
    return  decoded == source;
}

template<class Matrix>
int InteractiveCoder<Matrix>::errorCount(const std::vector<CodedVal> &decoded, const std::vector<CodedVal> &source)
{
    float numerrors = 0;
    for (int i = 0; i < decoded.size(); i++)
    {
        numerrors += std::abs(decoded[i] - source[i]);
    }

    return (int)numerrors;
}

template<class Matrix>
int InteractiveCoder<Matrix>::encode(uint32_t &io_numcode,  Syndrome &o_stored_syndrome, const std::vector<CodedVal> &source, uint32_t numcodeMax, uint32_t step, uint32_t n_ite)
{
    int n = source.size();

    std::vector<CodedVal> decoded(m_psyndromeMgr->get_n());

    uint32_t next_test_code = io_numcode;

    for(io_numcode = 0 ; io_numcode < numcodeMax ; io_numcode++)
    {
        m_psyndromeMgr->compute_stored(o_stored_syndrome, io_numcode, *llrProvider());

        if(io_numcode == next_test_code)
        {
            this->decode(decoded, io_numcode, n_ite, o_stored_syndrome);

            bool synd_test;

#ifdef TEST_SYNDROME
            if(m_decodage_syndrome->size() != m_psyndromeMgr->get_m(io_numcode))
            {
                std::cout << "error" << std::endl;
            }

            //not working for m > n/2
            for(int i = 0 ; i < m_decodage_syndrome->size(); i++)
            {
                synd_test = 0;
                for(const auto& vn: getMatrix(io_numcode).get_rows_from_col(i))
                {
                    synd_test ^= source[vn];
                }


                if(synd_test != m_decodage_syndrome->at(i))
                {
                    std::cout << "error" << std::endl;
                }
            }
#endif

            if(testConvergence(decoded, source))
            {
                return 0;
            }

//            std::cout << (1.0* this->m_psyndromeMgr->get_m(io_numcode)) / this->m_psyndromeMgr->get_n() << " " <<  errorCount(decoded, source) << std::endl;

            next_test_code = std::min(next_test_code+step, numcodeMax-1);
        }
    }

    //std::cout << io_numcode << std::endl;
    io_numcode = numcodeMax - 1;
    return errorCount(decoded, source);
}

template<class Matrix>
void InteractiveCoder<Matrix>::decode(std::vector<CodedVal> &o_decoded, uint32_t numcode, uint32_t n_ite, const Syndrome &stored_syndrome)
{
    m_psyndromeMgr->compute_deacc(*m_decodage_syndrome.get(), stored_syndrome, numcode, *llrProvider());
    decoder()->decode(o_decoded, m_psyndromeMgr->get_n(), numcode, n_ite, m_pllrProvider->llr(), *m_decodage_syndrome.get(), getMatrix(numcode));
}
