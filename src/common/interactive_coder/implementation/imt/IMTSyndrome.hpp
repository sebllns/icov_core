#include "IMTSyndrome.h"

using namespace icov;

void icov::IMTAccumulation::compute(const std::vector<CodedVal> &source_x, IMTTools &tool) const
{
    int n = tool.get_n();

    for(int i = 0; i < n/2 ; i++)
    {
        tool.syndrome12[i] = 0;
         //std::cout << i << ": ";
        for(const auto& vn: tool.h12_ord->get_rows_from_col(i))
        {
            //std::cout << vn << ", ";
            tool.syndrome12[i] ^= source_x[vn];
        }

        //std::cout << std::endl;
        if(i < n/4)
        {
            tool.syndrome14[i] = 0;
            for(const auto& vn: tool.h14_ord->get_rows_from_col(i))
            {
                tool.syndrome14[i] ^= source_x[vn];
            }
        }

        if(i < n/8)
        {
            tool.syndrome18[i] = 0;
            for(const auto& vn: tool.h18->get_rows_from_col(i))
            {
                tool.syndrome18[i] ^= source_x[vn];
            }
        }
    }
}

void icov::IMTDeaccumulation::compute(Syndrome &o_syndrome, const Syndrome &acc_syndrome, int numcode, IMTTools &tool, LLRProvider& llr_provider) const
{
    int m = tool.get_m(numcode);
    int n = tool.get_n();
    int m_18 = n/8;
    int m_14 = n/4;
    int m_12 = n/2;

    o_syndrome.resize(m);

    for (int i = 0; i < m; i++)
    {
        int k = i;

        if(m > m_14 && i < m_14)
        {
            k = tool.get_c18(i);
        }

        if(i > m_14 - m - 1 && i < m_18)
        {
            o_syndrome[k] = acc_syndrome[i] ^ acc_syndrome[m_14 - i - 1];
        }
        else
        {
            o_syndrome[k] = acc_syndrome[i];
        }       

        if(m > m_12 && k < m_12)
        {
            //tool.syndrome12[k] =  o_syndrome[k];
        }
    }

    for(int i = m_14 ; i < m && i < m_12 ; i++)
    {
        int ind = (m_12 - 1) - i;
        o_syndrome[ind] ^= o_syndrome[i];

        if(m > m_12 && ind < m_12)
        {
            //tool.syndrome12[ind] =  o_syndrome[ind];
        }
    }

    if(m > m_12)
    {
        //tool.compute_doping_order(llr_provider);
        for(int i = m_12; i < m; i++)
        {
            SIVal llr_val = (o_syndrome[i] ? -1.0 : 1.0) * llr_provider.limit_max();
            llr_provider[tool.doping_idx[i-m_12]] = llr_val;
        }
    }
}

void icov::IMTStoredSyndrome::compute(Syndrome &o_stored_syndrome, int numcode, IMTTools &tool, const LLRProvider& llr_provider) const
{
    int m = tool.get_m(numcode);
    o_stored_syndrome.resize(m);

    int m_12 = tool.get_n()/2;

    if(m == tool.get_n()/8)
    {
        for(int i = 0; i < m ; i++)
        {
            o_stored_syndrome[i] = tool.syndrome18[i];
        }
    }
    else if(m <= m_12)
    {
        o_stored_syndrome[m-1] = m > tool.get_n()/4 ? tool.syndrome12[m-1] : tool.syndrome14[m-1];
    }
    else
    {
        tool.compute_doping_order(llr_provider);
        o_stored_syndrome[m-1] = tool.source[tool.doping_idx[m-m_12-1]];
    }
}
