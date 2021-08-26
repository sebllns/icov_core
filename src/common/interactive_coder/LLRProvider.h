#ifndef LLRPROVIDER_H
#define LLRPROVIDER_H

#include "common_types.h"

namespace icov {

class LLRProvider
{
public:
    LLRProvider(int n);
    LLRProvider(const std::vector<SIVal> &llr);

    inline SIVal& operator [] (int i) {return m_llr[i];}
    inline SIVal operator [] (int i) const {return m_llr[i];}
    inline const std::vector<SIVal>& llr() const {return m_llr;}
    inline const CodedVal& src(int i) const {return m_src[i];}

    void initLLR(float pCrossover, const std::vector<CodedVal>& src);
    void initLLRUniform(float pCrossover, const std::vector<CodedVal>& src);

    uint32_t index_min() const;
    static SIVal limit_max();

protected:
    std::vector<SIVal> m_llr;
    std::vector<CodedVal> m_src;
    std::vector<size_t> doping_order;
    bool m_floor_type;
};

}

#endif // LLRPROVIDER_H
