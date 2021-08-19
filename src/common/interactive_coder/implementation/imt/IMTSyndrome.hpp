#include "IMTSyndrome.h"

using namespace icov;


inline IMTSyndromeManager::IMTSyndromeManager(int n): ISyndromeManager(n), doping_idx(n), init_rateless(true)
{
    std::iota(doping_idx.begin(), doping_idx.end(), 0);
}

inline int IMTSyndromeManager::get_m(int numcode) const
{
    return  this->get_n()/8 + numcode;
}

inline void IMTSyndromeManager::reset_doping() { init_rateless = true;}

inline u_int32_t IMTSyndromeManager::get_c18(int m) const
{
    if(m >= get_n()/8)
    {
        return c18->at(get_n()/4 - m - 1,1) - 1;
    }

    return c18->at(m,0) - 1;
}

inline void IMTSyndromeManager::compute_doping_order(const LLRProvider &llr_provider)
{
    if(!init_rateless) return;
    std::cout << "compute doping" << std::endl;
    Aff3ctDecoder dec;
    std::vector<CodedVal> o_decoded(this->get_n());
    int numcode = this->get_n()/2;
    dec.decode_doping(o_decoded, doping_idx, this->get_n(), numcode, 100, llr_provider.llr(), syndrome12, *h12_ord);
    init_rateless = false;
}


void IMTSyndromeManager::init_accumulation(const std::vector<CodedVal>& source_x)
{
    int n = get_n();

    for(int i = 0; i < n/2 ; i++)
    {
        syndrome12[i] = 0;
         //std::cout << i << ": ";
        for(const auto& vn: h12_ord->get_rows_from_col(i))
        {
            //std::cout << vn << ", ";
            syndrome12[i] ^= source_x[vn];
        }

        //std::cout << std::endl;
        if(i < n/4)
        {
            syndrome14[i] = 0;
            for(const auto& vn: h14_ord->get_rows_from_col(i))
            {
                syndrome14[i] ^= source_x[vn];
            }
        }

        if(i < n/8)
        {
            syndrome18[i] = 0;
            for(const auto& vn: h18->get_rows_from_col(i))
            {
                syndrome18[i] ^= source_x[vn];
            }
        }
    }
}

void IMTSyndromeManager::compute_deacc(Syndrome& o_syndrome, const Syndrome& acc_syndrome, int numcode, LLRProvider& llr_provider)
{
    int m = this->get_m(numcode);
    int n = get_n();
    int m_18 = n/8;
    int m_14 = n/4;
    int m_12 = n/2;

    o_syndrome.resize(m);

    for (int i = 0; i < m; i++)
    {
        int k = i;

        if(m > m_14 && i < m_14)
        {
            k = get_c18(i);
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
          //  syndrome12[k] =  o_syndrome[k];
        }
    }

    for(int i = m_14 ; i < m && i < m_12 ; i++)
    {
        int ind = (m_12 - 1) - i;
        o_syndrome[ind] ^= o_syndrome[i];

        if(m > m_12 && ind < m_12)
        {
           // syndrome12[ind] =  o_syndrome[ind];
        }
    }

    if(m > m_12)
    {
        //compute_doping_order(llr_provider);
        for(int i = m_12; i < m; i++)
        {
            SIVal llr_val = (o_syndrome[i] ? -1.0 : 1.0) * llr_provider.limit_max();
            llr_provider[doping_idx[i-m_12]] = llr_val;
        }
    }
}

void IMTSyndromeManager::compute_stored(Syndrome& o_stored_syndrome, int numcode, const LLRProvider& llr_provider)
{
    int m = this->get_m(numcode);
    o_stored_syndrome.resize(m);

    int m_12 = get_n()/2;

    if(m == get_n()/8)
    {
        for(int i = 0; i < m ; i++)
        {
            o_stored_syndrome[i] = syndrome18[i];
        }
    }
    else if(m <= m_12)
    {
        o_stored_syndrome[m-1] = m > get_n()/4 ? syndrome12[m-1] : syndrome14[m-1];
    }
    else
    {
        compute_doping_order(llr_provider);
        o_stored_syndrome[m-1] = source[doping_idx[m-m_12-1]];
    }
}
