#include "VarodayanSyndrome.h"

using namespace icov;

void icov::VarodayanAccumulation::compute(const std::vector<CodedVal> &source_x, VarodayanTools &tool) const
{
    int numcode = tool.numCodes-1;
    int n = tool.get_n();

    int k  = 0;
    int l = 0;

    int m = tool.get_m(numcode);

    tool.codeSumVec = std::vector<uint32_t>(tool.numIncVec.size());
    tool.codeSumVec[0] = 0;
    for(int c = 1; c < (int)tool.codeSumVec.size(); c++)
    {
        tool.codeSumVec[c] = tool.codeSumVec[c-1] + tool.numIncVec[c-1];
    }

    tool.accumulated_syndrome = Syndrome(n);

    for (k = 0; k < m; k++)
        tool.accumulated_syndrome[k] = 0;

    //source*H'
    for (k = 0; k < n; k++)
    {
        for (l = tool.jc[k]; l < (int)tool.jc[k + 1]; l++)
        {
            int ir = tool.getIr(tool.numCodes-1, l);
             tool.accumulated_syndrome[ir] ^= source_x[k];
        }
    }

    //accumulate
    for (k = 1; k < m; k++)
    {
         tool.accumulated_syndrome[k] ^=  tool.accumulated_syndrome[k - 1];
    }
}

void icov::VarodayanDeaccumulation::compute(Syndrome &o_syndrome, const Syndrome &acc_syndrome, int numcode, const VarodayanTools &tool) const
{
    int m = tool.get_m(numcode);
    o_syndrome.resize(m);
    o_syndrome[0] = acc_syndrome[0];
    for (int k = 1; k < (int)m; k++)
    {
        //CALCUL SYNDROME
        o_syndrome[k] = acc_syndrome[k] ^ acc_syndrome[k-1];
    }
}

void icov::VarodayanStoredSyndrome::compute(Syndrome &o_stored_syndrome, int numcode, const VarodayanTools &tool) const
{
    int numInc = tool.numIncVec[numcode];
    int codeSum = tool.codeSumVec[numcode];

    int currIndex = 0;

    int m = tool.get_m(numcode);

    o_stored_syndrome.resize(m);

//    std::cout<<"accsynd\n";
//    std::cout<< tool.accumulated_syndrome.size() << std::endl;

//    for(int i = 0; i < tool.accumulated_syndrome.size() ; i++)
//    {
//        std::cout<< tool.accumulated_syndrome.at(i) << ", ";
//    }
//    std::cout<< std::endl;


    //std::cout<<"stored\n";
    //std::cout<< o_stored_syndrome.size() << std::endl;

    for (int k = 0; k < (int)m; k++)
    {
        currIndex = tool.txSeq[codeSum + k % numInc] + (k / numInc) * tool.totalNumInc;
        o_stored_syndrome[k] = tool.accumulated_syndrome[currIndex];//OK
         //std::cout<< currIndex << ", ";
    }
   // std::cout<< std::endl;
}
