#ifndef ISYNDROME_H
#define ISYNDROME_H

#include "common_types.h"
#include "interactive_coder/LLRProvider.h"

namespace icov
{

class ISyndromeManager
{
public:
    ISyndromeManager(int _n) : n(_n){};
    virtual ~ISyndromeManager() = default;

    virtual void init_accumulation(const std::vector<CodedVal>& source_x) = 0;
    virtual void compute_stored(Syndrome& o_stored_syndrome, int numcode, const LLRProvider& llr_provider) = 0;

    virtual void compute_deacc(Syndrome& o_syndrome, const Syndrome& acc_syndrome, int numcode, LLRProvider& llr_provider) = 0;

    virtual int get_m(int numcode) const
    {
        return numcode;
    }

    virtual int get_n() const
    {
        return this->n;
    }

protected:
    uint32_t n;
};
}

#endif // ISYNDROME_H
