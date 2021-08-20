#include "VarodayanSyndrome.h"

using namespace icov;

inline VarodayanSyndromeManager::VarodayanSyndromeManager(int n):
    ISyndromeManager(n)
{

}

inline int VarodayanSyndromeManager::get_m(int numcode) const
{
    int numInc = this->numIncVec[numcode];
    return (this->get_n() / totalNumInc) * numInc;
}


inline uint32_t VarodayanSyndromeManager::getIr(int code, int k) const
{
    return  ir[code * nzmax + k];
}


inline void VarodayanSyndromeManager::read_mat(std::ifstream &file)
{
    //1- PARSE VARODAYAN FILE
    uint32_t k = 0;
    uint32_t v = 0;

    file >> this->numCodes;
    file >> this->n;
    file >> this->nzmax;
    file >> this->totalNumInc;

    std::cout << numCodes << std::endl;
    std::cout << n << std::endl;
    std::cout << nzmax << std::endl;
    std::cout << totalNumInc << std::endl;

    for (k = 0; k < this->n + 1; k++)
    {
        file >> v;
        this->jc.push_back(v);
    }

    for (uint32_t code = 0; code < this->numCodes; code++)
    {
        file >> v;
        this->numIncVec.push_back(v);

        for (k = 0; k < this->numIncVec[code]; k++)
        {
            file >> v;
            this->txSeq.push_back(v);
        }

        for (k = 0; k < this->nzmax; k++)
        {
            file >> v;
            this->ir.push_back(v);
        }
    }
}


inline void VarodayanSyndromeManager::read_mat(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::stringstream message;
        message << "'filename' couldn't be opened ('filename' = " << filename << ").";
        throw message.str();
    }

    read_mat(file);
}

void VarodayanSyndromeManager::init_accumulation(const std::vector<CodedVal> &source_x)
{
    int numcode = numCodes-1;
    int n = get_n();

    int k  = 0;
    int l = 0;

    int m = get_m(numcode);

    codeSumVec = std::vector<uint32_t>(numIncVec.size());
    codeSumVec[0] = 0;
    for(int c = 1; c < (int)codeSumVec.size(); c++)
    {
        codeSumVec[c] = codeSumVec[c-1] + numIncVec[c-1];
    }

    accumulated_syndrome = Syndrome(n);

    for (k = 0; k < m; k++)
        accumulated_syndrome[k] = 0;

    //source*H'
    for (k = 0; k < n; k++)
    {
        for (l = jc[k]; l < (int)jc[k + 1]; l++)
        {
            int ir = getIr(numCodes-1, l);
             accumulated_syndrome[ir] ^= source_x[k];
        }
    }

    //accumulate
    for (k = 1; k < m; k++)
    {
         accumulated_syndrome[k] ^=  accumulated_syndrome[k - 1];
    }
}

void VarodayanSyndromeManager::compute_deacc(Syndrome &o_syndrome, const Syndrome &acc_syndrome, int numcode, LLRProvider &llr_provider)
{
    int m = get_m(numcode);
    o_syndrome.resize(m);
    o_syndrome[0] = acc_syndrome[0];
    for (int k = 1; k < (int)m; k++)
    {
        //CALCUL SYNDROME
        o_syndrome[k] = acc_syndrome[k] ^ acc_syndrome[k-1];
    }
}

void VarodayanSyndromeManager::compute_stored(Syndrome &o_stored_syndrome, int numcode, const LLRProvider &llr_provider)
{
    int numInc = numIncVec[numcode];
    int codeSum = codeSumVec[numcode];

    int currIndex = 0;

    int m = get_m(numcode);

    o_stored_syndrome.resize(m);

//    std::cout<<"accsynd\n";
//    std::cout<< accumulated_syndrome.size() << std::endl;

//    for(int i = 0; i < accumulated_syndrome.size() ; i++)
//    {
//        std::cout<< accumulated_syndrome.at(i) << ", ";
//    }
//    std::cout<< std::endl;


    //std::cout<<"stored\n";
    //std::cout<< o_stored_syndrome.size() << std::endl;

    for (int k = 0; k < (int)m; k++)
    {
        currIndex = txSeq[codeSum + k % numInc] + (k / numInc) * totalNumInc;
        o_stored_syndrome[k] = accumulated_syndrome[currIndex];//OK
         //std::cout<< currIndex << ", ";
    }
   // std::cout<< std::endl;
}
