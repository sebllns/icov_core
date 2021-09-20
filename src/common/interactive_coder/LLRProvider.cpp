#include "LLRProvider.h"

#include <iostream>
#include <cmath>
#include <limits>
#include <algorithm>
#include <random>111

icov::LLRProvider::LLRProvider(int n):
    m_llr(n, 0)
{
    std::iota(doping_order.begin(), doping_order.end(), 0);
}

icov::LLRProvider::LLRProvider(const std::vector<LLRVal> &llr):
    m_llr(llr)
{
    std::iota(doping_order.begin(), doping_order.end(), 0);
}

void icov::LLRProvider::initLLR(int b, BitSymbolProvider& bsp, const std::vector<SIVal> &side_info, const std::vector<int> &predec)
{
    int l = bsp.z_binary_length();
    float p = 0.0, p0 = 0.0, p1 = 0.0;

    for(int i = 0; i < side_info.size() ; i++)
    {
        m_llr[i] = b == 0 ? p = bsp.pqks(side_info[i]) : bsp.pqkb(predec[i] < 0, b > 1 ? std::abs(predec[i]) : 0, l, b, side_info[i], p0, p1, p);
        m_llr[i] = std::log(m_llr[i] / (1 - m_llr[i]));
    }
}

void icov::LLRProvider::initLLR(float pcrossover, const std::vector<CodedVal> &src_bits)
{
    if(m_llr.size() != src_bits.size()) throw "LLRProvider::initLLR: length of arrays are not equal";

    m_src = src_bits;

    for(int k = 0 ; k < (int)src_bits.size() ; k++)
    {
        int sideinfo_BSC = ((int) (src_bits[k] + (((((double)rand() / (double)RAND_MAX) < pcrossover)) ? 1.0 : 0.0))) % 2;
        double pCond = (1-pcrossover)*(1-sideinfo_BSC) + pcrossover*sideinfo_BSC;
        m_llr[k] = std::log( pCond/(1-pCond) );
    }

}

void icov::LLRProvider::initLLRUniform(float pcrossover, const std::vector<CodedVal> &src_bits)
{
    if(m_llr.size() != src_bits.size()) throw "LLRProvider::initLLRUniform length of arrays are not equal";

    m_src = src_bits;

    float one_count = pcrossover * m_llr.size();
    one_count = m_floor_type ? std::ceil(one_count) : std::floor(one_count);

    for(int k = 0 ; k < (int)src_bits.size() ; k++)
    {
        m_llr[k] = k < one_count ? 1 : 0;
    }

    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(m_llr.begin(), m_llr.end(), g);

    for(int k = 0 ; k < (int)src_bits.size() ; k++)
    {
        m_llr[k] = (int)(m_llr[k] + src_bits[k]) % 2;
        double pCond = (1-pcrossover)*(1-m_llr[k]) + pcrossover*m_llr[k];
        m_llr[k] = log( pCond/(1-pCond) );
    }

    m_floor_type = !m_floor_type;
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

icov::LLRVal icov::LLRProvider::limit_max()
{
    return std::numeric_limits<LLRVal>::max();
}
