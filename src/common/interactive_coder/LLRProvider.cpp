#include "LLRProvider.h"

#include <iostream>
#include <cmath>
#include <limits>

icov::LLRProvider::LLRProvider(int n):
    m_llr(n, 0)
{
    std::iota(doping_order.begin(), doping_order.end(), 0);
}

icov::LLRProvider::LLRProvider(const std::vector<SIVal> &llr):
    m_llr(llr)
{
    std::iota(doping_order.begin(), doping_order.end(), 0);
}

void icov::LLRProvider::initLLR(float pcrossover, const std::vector<CodedVal> &src_bits)
{
    if(m_llr.size() != src_bits.size()) throw "LLRProvider::buildLLR length of arrays are not equal";

    m_src = src_bits;

    for(int k = 0 ; k < (int)src_bits.size() ; k++)
    {
        int sideinfo_BSC = ((int) (src_bits[k] + (((((double)rand() / (double)RAND_MAX) < pcrossover)) ? 1.0 : 0.0))) % 2;
        double pCond = (1-pcrossover)*(1-sideinfo_BSC) + pcrossover*sideinfo_BSC;
        m_llr[k] = log( pCond/(1-pCond) );
    }

}

uint32_t icov::LLRProvider::index_min() const
{
    uint32_t idx = 0;

    for(int i= 0; i < (int)m_llr.size() ; i++)
    {
        if(std::fabs(m_llr[i]) < std::fabs(m_llr[idx]))
        {
            idx = i;
        }
    }

    //std::cout << idx << ", ";

    return idx;
}

icov::SIVal icov::LLRProvider::limit_max()
{
    return std::numeric_limits<SIVal>::max();
}
