#include "LLRProvider.h"

#include <iostream>
#include <cmath>
#include <limits>
#include <algorithm>
#include <random>

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

void icov::LLRProvider::initLLR_s(BitSymbolProvider &bsp, const std::vector<SIVal> &side_info, const std::vector<CodedVal> &src_bits)
{
    m_src = src_bits;
    for(int i = 0; i < side_info.size() ; i++)
    {
        double p = bsp.pqks(side_info[i]);

        p = std::clamp(p, (double)0.000000001, (double)(1.0 - 0.000000001));

        //std::cout << p << std::endl;

        if(p == 0 || p == 1) std::cerr << "initLLR_s: p cannot be 0 or 1";
        m_llr[i] = std::log(p / (1.0 - p));

       std::cout << m_llr[i] << " ";

    }

    std::cout<<std::endl;
}

void icov::LLRProvider::initLLR_b(int b, BitSymbolProvider &bsp, const std::vector<SIVal> &side_info, const std::vector<int> sign,  const std::vector<int>& abs_predec, const std::vector<CodedVal> &src_bits)
{
    m_src = src_bits;

    int l = bsp.x_binary_length();
    float p = 0.0, p0 = 0.0, p1 = 0.0;

    for(int i = 0; i < side_info.size() ; i++)
    {
       bsp.pqkb(sign[i] > 0, abs_predec[i], l, b, side_info[i], p0, p1, p);
       p = std::clamp(p, (float)0.000001, (float)(1.0 - 0.000001));
       m_llr[i] = std::log(p / (1.0 - p));
       /*if(p == 0 || p == 1)
       {
           std::cerr << "initLLR_b: p cannot be 0 or 1";
       }*/
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


