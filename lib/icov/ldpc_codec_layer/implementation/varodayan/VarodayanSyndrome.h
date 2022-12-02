#ifndef VARODAYANSYNDROME_H
#define VARODAYANSYNDROME_H

#include "interface/ISyndrome.h"
#include <fstream>
#include <sstream>

namespace icov {

class VarodayanSyndromeManager : public ISyndromeManager {
public:
  uint32_t nzmax;
  uint32_t numCodes;
  uint32_t totalNumInc;

  std::vector<uint32_t> numIncVec;
  std::vector<uint32_t> codeSumVec;
  std::vector<uint32_t> txSeq;
  std::vector<uint32_t> jc;
  std::vector<uint32_t> ir;

  Syndrome accumulated_syndrome;

  [[deprecated]] VarodayanSyndromeManager(int n);
  virtual ~VarodayanSyndromeManager() = default;

  uint32_t getIr(int code, int k) const;
  void read_mat(std::ifstream &file);
  void read_mat(const std::string &filename);

  // ISyndromeManager interface
  virtual int get_m(int numcode) const override;
  virtual void init_accumulation(const BitVec &source_x) override;
  virtual void compute_stored(Syndrome &o_stored_syndrome, int numcode,
                              LLRProvider &llr_provider,
                              const BitVec &src) override;
  virtual void compute_deacc(Syndrome &o_syndrome, const Syndrome &acc_syndrome,
                             int numcode, LLRProvider &llr_provider) override;

  std::string get_type() const override { return "varodayan"; }
};

} // namespace icov
#endif // VARODAYANSYNDROME_H
