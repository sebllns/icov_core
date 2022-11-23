#ifndef LLRPROVIDER_H
#define LLRPROVIDER_H

#include "BitSymbolProvider.h"
#include "common.h"

namespace icov {

class LLRProvider {
public:
  LLRProvider(int n);
  LLRProvider(const std::vector<LLRVal> &llr);

  inline LLRVal &operator[](int i) { return m_llr[i]; }
  inline LLRVal operator[](int i) const { return m_llr[i]; }
  inline const std::vector<LLRVal> &llr() const { return m_llr; }
  //  inline const BitVec &src() const { return m_src; }
  //  inline const BitVal &src(int i) const { return m_src[i]; }

  void initLLR(const std::vector<SIVal> &side_info, const BitVec &src_bits);
  void initLLR(float pCrossover, const BitVec &src);
  void initLLRUniform(float pCrossover, const BitVec &src);
  std::vector<icov::BitVal> initSrcRand(int n);

  uint32_t index_min() const;
  static LLRVal limit_max();
  static LLRVal epsilon();

  void reset();

  void initLLR_s(const BitSymbolProvider &bsp,
                 const std::vector<SIVal> &side_info);
  void initLLR_b(int b, const BitSymbolProvider &bsp,
                 const std::vector<SIVal> &side_info,
                 const std::vector<int> &sign,
                 const std::vector<int> &abs_predec);

  bool no_coding(const BitVec &src) const;

protected:
  std::vector<LLRVal> m_llr;
  //  BitVec m_src;
  std::vector<size_t> doping_order;
  bool m_floor_type;
};

} // namespace icov

#endif // LLRPROVIDER_H
