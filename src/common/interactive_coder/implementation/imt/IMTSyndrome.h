#ifndef IMTSYNDROME_H
#define IMTSYNDROME_H

#include "interactive_coder/interface/ISyndrome.h"


#include <aff3ct.hpp>
#include "interactive_coder/aff3ct/Aff3ctDecoder.h"

using namespace aff3ct;

using AffMatrix = tools::Sparse_matrix;


namespace icov {

class IMTSyndromeManager : public ISyndromeManager
{

public:

    AffMatrix *h12_ord;
    AffMatrix *h14_ord;
    AffMatrix *h18;

    Syndrome syndrome12;
    Syndrome syndrome14;
    Syndrome syndrome18;

    std::vector<size_t> doping_idx;

    FastMatrix<uint32_t> *c18;

    bool init_rateless;


    IMTSyndromeManager(int n);
    virtual ~IMTSyndromeManager() = default;

    virtual void init_accumulation(const std::vector<CodedVal>& source_x) override;

    virtual void compute_stored(Syndrome& o_stored_syndrome, int numcode, const LLRProvider& llr_provider) override;

    virtual void compute_deacc(Syndrome& o_syndrome, const Syndrome& acc_syndrome, int numcode, LLRProvider& llr_provider) override;

    virtual int get_m(int numcode) const override;

    void reset_doping() override;

    uint32_t get_c18(int m) const;

    void compute_doping_order(const LLRProvider& llr_provider);

    std::string get_type() const override;


};
}

#endif // IMTSYNDROME_H
