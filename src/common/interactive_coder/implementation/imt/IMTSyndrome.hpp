#include "IMTSyndrome.h"

using namespace icov;


IMTSyndromeManager::IMTSyndromeManager(int n): ISyndromeManager(n), doping_idx(n), init_rateless(true)
{
    std::iota(doping_idx.begin(), doping_idx.end(), 0);
}

int IMTSyndromeManager::get_m(int numcode) const
{
    //m is given by the numcode with an offset of n/8 (minimum rate)
    return  this->get_n()/8 + numcode;
}

void IMTSyndromeManager::reset_doping() { init_rateless = true;}

uint32_t IMTSyndromeManager::get_c18(int m) const
{
    //Matrix c18 is given by the IMT mapping file of size n/4 x 2
    //the row in the second half of the matrix matches the second column of the c18 mapping file but in reverse order
    if(m >= get_n()/8)
    {
        return c18->at(get_n()/4 - m - 1,1) - 1;
    }

    //the row in the first half of the matrix matches the first column of the c18 mapping file
    return c18->at(m,0) - 1;
}

void IMTSyndromeManager::compute_doping_order(const LLRProvider &llr_provider)
{
    if(!init_rateless) return;
    //std::cout << "compute doping" << std::endl;
    Aff3ctDecoder dec(0);
    std::vector<CodedVal> o_decoded(this->get_n());
    int numcode = this->get_n()/2;
    dec.decode_doping(o_decoded, doping_idx, this->get_n(), numcode, 100, llr_provider.llr(), syndrome12, *h12_ord);
    init_rateless = false;
}


void IMTSyndromeManager::init_accumulation(const std::vector<CodedVal>& source_x)
{
    int n = get_n();

    //We fill the 3 syndrome vectors directly calculated from source for m=n/2, m=n/4, m=n/8
    for(int i = 0; i < n/2 ; i++)
    {
        //compute syndrome12 = h12_ord * x
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
            //compute syndrome14 = h14_ord * x
            syndrome14[i] = 0;
            for(const auto& vn: h14_ord->get_rows_from_col(i))
            {
                syndrome14[i] ^= source_x[vn];
            }
        }

        if(i < n/8)
        {
            //compute syndrome18 = h18 * x
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

        //When m_18 <= m < m_14, we use h14_ord for decoding => no need to reorder.
        int k = i;

        //When m_14 < m < m_12, we need to permute rows according to c18 mapping order,
        //since we are using h14 for decoding.
        if(m > m_14 && i < m_14)
        {
            k = get_c18(i);
        }

        if(i > m_14 - m - 1 && i < m_18)
        {
            //First and last row are accumulated, we repeat that incremental process going from m_18 to m_14
            o_syndrome[k] = acc_syndrome[i] ^ acc_syndrome[m_14 - i - 1];
        }
        else
        {
            o_syndrome[k] = acc_syndrome[i];
        }       

        //we fill the syndrome12 when decoding, only needed for doping
        if(m > m_12 && k < m_12)
        {
            syndrome12[k] =  o_syndrome[k];
        }
    }

    for(int i = m_14 ; i < m && i < m_12 ; i++)
    {
        //deaccumulation process from m_14 to m_12
        int ind = (m_12 - 1) - i;
        o_syndrome[ind] ^= o_syndrome[i];

        //we fill the syndrome12 when decoding, only needed for doping
        if(m > m_12 && ind < m_12)
        {
            syndrome12[ind] =  o_syndrome[ind];
        }
    }

    //doping for rate > 0.5
    if(m > m_12)
    {
        //order the llr values for rate = 0.5
        compute_doping_order(llr_provider);

        //put all m-m_12 lowest LLR to the max value (keeping sign)
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
    int m_18 = get_n()/8;

    //intial syndrome is syndrome18
    if(m == m_18)
    {
        for(int i = 0; i < m_18 ; i++)
        {
            o_stored_syndrome[i] = syndrome18[i];
        }
    }
    else if(m <= m_12)
    {
        //incremental syndrome
        o_stored_syndrome[m-1] = m > get_n()/4 ? syndrome12[m-1] : syndrome14[m-1];
    }
    else
    {
        //doping when rate > 0.5
        compute_doping_order(llr_provider);
        o_stored_syndrome[m-1] = llr_provider.src(doping_idx[m-m_12-1]);
    }
}
