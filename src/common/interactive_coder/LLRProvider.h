#ifndef LLRPROVIDER_H
#define LLRPROVIDER_H

#include "common.h"
#include "BitSymbolProvider.h"

namespace icov {

class LLRProvider
{
public:
    LLRProvider(int n);
    LLRProvider(const std::vector<LLRVal> &llr);

    inline LLRVal& operator [] (int i) {return m_llr[i];}
    inline LLRVal operator [] (int i) const {return m_llr[i];}
    inline const std::vector<LLRVal>& llr() const {return m_llr;}
    inline const std::vector<CodedVal>& src() const {return m_src;}
    inline const CodedVal& src(int i) const {return m_src[i];}

    void initLLR(const std::vector<SIVal> &side_info , const std::vector<CodedVal> &src_bits);
    void initLLR(float pCrossover, const std::vector<CodedVal>& src);
    void initLLRUniform(float pCrossover, const std::vector<CodedVal>& src);
    std::vector<icov::CodedVal> initSrcRand();

    uint32_t index_min() const;
    static LLRVal limit_max();

     void initLLR_s(BitSymbolProvider &bsp, const std::vector<SIVal> &side_info, const std::vector<CodedVal> &src_bits);
     void initLLR_b(int b, BitSymbolProvider &bsp, const std::vector<SIVal> &side_info, const std::vector<int> sign,  const std::vector<int>& abs_predec, const std::vector<CodedVal> &src_bits);

     bool no_coding() const;

protected:
    std::vector<LLRVal> m_llr;
    std::vector<CodedVal> m_src;
    std::vector<size_t> doping_order;
    bool m_floor_type;
};

}

#endif // LLRPROVIDER_H
