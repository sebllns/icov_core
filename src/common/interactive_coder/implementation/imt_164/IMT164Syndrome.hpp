#include "IMT164Syndrome.h"

using namespace icov;


IMT164SyndromeManager::IMT164SyndromeManager(int n, int min_level, int max_level): ISyndromeManager(n),
    doping_idx(n), init_rateless(true), m_min_level(min_level), m_max_level(max_level)
{
    std::iota(doping_idx.begin(), doping_idx.end(), 0);
    int l = m_min_level;
    while (l <= m_max_level)
    {
        m_levels.insert(m_levels.begin(), l);
        l *= 2;
    }

    this->h.resize(get_level_count());
    this->mapping_file.resize(get_level_count()-1, FastMatrix<index_t>(0,0));

    accumulated_syndrome = nullptr;
}

int IMT164SyndromeManager::get_m_max() const
{
    return get_m_level(m_min_level);
}

int IMT164SyndromeManager::get_m_min() const
{
    return get_m_level(m_max_level);
}

int IMT164SyndromeManager::get_m_level(int level) const
{
    return get_n()/level;
}

int IMT164SyndromeManager::get_level_count() const
{
    return m_levels.size();
}


int IMT164SyndromeManager::get_level(int k) const
{
    return m_levels[k];
}

int IMT164SyndromeManager::get_numcode(int m) const
{
    return m - this->get_m_min();
}

int IMT164SyndromeManager::is_level(int m) const
{
    for(int i = 0 ; i < get_level_count() ; ++i)
    {
        if(get_m_level(get_level(i)) == m)
        {
            return i;
        }
    }

    return -1;
}

int IMT164SyndromeManager::get_m(int numcode) const
{
    //m is given by the numcode with an offset of n/8 (minimum rate)
    return  this->get_m_min() + numcode;
}

void IMT164SyndromeManager::reset_doping() { init_rateless = true;}


void IMT164SyndromeManager::compute_doping_order(const LLRProvider &llr_provider, Syndrome &syndrome)
{
    if(!init_rateless) return;
    //std::cout << "compute doping" << std::endl;
    Aff3ctDecoder dec(0);
    std::vector<CodedVal> o_decoded(this->get_n());
    int numcode = this->get_m_max();
    dec.decode_doping(o_decoded, doping_idx, this->get_n(), numcode, 100, llr_provider.llr(), syndrome, h[this->get_level_count()-1]);
    init_rateless = false;
}


void IMT164SyndromeManager::init_accumulation(const std::vector<CodedVal>& source_x)
{
    //std::cout << "init_accumulation" << std::endl;

    if(accumulated_syndrome == nullptr)
    {
        accumulated_syndrome = new icov::bit[this->get_m_max() + 1];
    }
    accumulated_syndrome[0] = 0;

    int offset = 1;

    for(int k = 0; k < get_level_count() ; k++)
    {
        int m = get_m_level(m_levels[k]);
        int count = k > 0 ? m/2 : m;

        //We fill the 3 syndrome vectors directly calculated from source for m=n/2, m=n/4, m=n/8
        for(int i = 0; i < count ; i++)
        {
            int row = k > 0 ? mapping_file[k-1].at(i, 0) : i;

            //compute syndrome = h * x
            icov::bit val = 0;
            for(const auto& vn: h[k].get_rows_from_col(row))
            {
                val ^= source_x[vn];
            }

           accumulated_syndrome[i + offset] = val;
        }

        offset += count;
    }

    if(offset != (get_m_max()+1))
    {
        throw new std::runtime_error("error offset != n/2");
    }

//    doping_synd = Syndrome(get_m_max());
//    for(int i = 0; i < get_m_max() ; i++)
//    {
//        //compute syndrome = h * x
//        icov::bit val = 0;
//        for(const auto& vn: h[get_level_count()-1]->get_rows_from_col(i))
//        {
//            val ^= source_x[vn];
//        }

//       doping_synd[i] = val;
//    }

    //h*x
    synd_dec.clear();
    for(int m = get_m_min() ; m < get_m_max()+1 ; m++)
    {
        Syndrome s(m);
        for(int i = 0 ; i < m; i++)
        {
            bool synd_test = 0;
            for(const auto& vn: h_dec[get_numcode(m)].get_rows_from_col(i))
            {
                synd_test ^= source_x[vn];
            }

            s.at(i) = synd_test;

        }
        synd_dec.push_back(s);
    }

    doping_synd = synd_dec[synd_dec.size()-1];
}

void IMT164SyndromeManager::compute_deacc(Syndrome& o_syndrome, const Syndrome& acc_syndrome, int numcode, LLRProvider& llr_provider)
{

    int m = get_m(numcode);
    int n = get_n();

    //std::cout << "compute_deacc " << m << std::endl;

    o_syndrome.resize(std::min(m, get_m_max()));

//    for (int i = 0; i < std::min(m, get_m_max()); i++)
//    {
//        o_syndrome[i] = 0;

//        for(const auto &k: this->m_decoded_syndrome[std::min(numcode, (int)m_decoded_syndrome.size()-1)][i])
//        {
//            o_syndrome[i] ^= this->accumulated_syndrome[k];
//        }
//    }


        for (int i = 0; i < std::min(m, get_m_max()); i++)
        {
            o_syndrome[i] = synd_dec[std::min(numcode, (int)synd_dec.size()-1)][i];
        }


    //doping for rate > 0.5
    if(m >  get_m_max())
    {
        //std::cout << "doping" << std::endl;

        //order the llr values for rate = 0.5
        compute_doping_order(llr_provider, doping_synd);

        o_syndrome.resize(m);

        //put all m-m_12 lowest LLR to the max value (keeping sign)
        for(int i =  get_m_max(); i < m; i++)
        {
            o_syndrome[i] =  acc_syndrome[i];
            LLRVal llr_val = (o_syndrome[i] ? -1.0 : 1.0) * llr_provider.limit_max();
            llr_provider[doping_idx[i- get_m_max()]] = llr_val;
        }
    }
}

void IMT164SyndromeManager::compute_stored(Syndrome& o_stored_syndrome, int numcode, const LLRProvider& llr_provider)
{
    //std::cout << "compute stored" << std::endl;
    int m = get_m(numcode);
    o_stored_syndrome.resize(m);

    //intial syndrome is syndrome18
    if(m == get_m_min())
    {
        for(int i = 0; i < get_m_min() ; i++)
        {
            o_stored_syndrome[i] = this->accumulated_syndrome[i+1];
        }
    }
    else if(m <= get_m_max())
    {
        //incremental syndrome
        o_stored_syndrome[m-1] = this->accumulated_syndrome[m];
    }
    else
    {
        //doping when rate > 0.5

        if(init_rateless)
        {
            compute_doping_order(llr_provider, doping_synd);
        }

        o_stored_syndrome[m-1] = llr_provider.src(doping_idx[m-get_m_max()-1]);
    }
}
