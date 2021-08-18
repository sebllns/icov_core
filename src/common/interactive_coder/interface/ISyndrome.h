#ifndef ISYNDROME_H
#define ISYNDROME_H

#include "common_types.h"
#include "interactive_coder/LLRProvider.h"

namespace icov
{

struct DefaultSyndromeTools
{
public:
    DefaultSyndromeTools() = default;
    virtual ~DefaultSyndromeTools() = default;

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

template <class SyndromeTools = DefaultSyndromeTools>
class ISyndromeAccumulation
{
public:
    ISyndromeAccumulation() = default;
    virtual ~ISyndromeAccumulation() = default;

    virtual void compute(const std::vector<CodedVal>& source_x, SyndromeTools& tool) const = 0;
};

template <class SyndromeTools = DefaultSyndromeTools>
class ISyndromeEncodage
{
public:
    ISyndromeEncodage() = default;
    virtual ~ISyndromeEncodage() = default;

    virtual void compute(Syndrome& o_stored_syndrome, int numcode, SyndromeTools& tool, const LLRProvider& llr_provider) const = 0;
};


template <class SyndromeTools = DefaultSyndromeTools>
class ISyndromeDecodage
{
public:
    ISyndromeDecodage() = default;
    virtual ~ISyndromeDecodage() = default;
    virtual void compute(Syndrome& o_syndrome, const Syndrome& acc_syndrome, int numcode, SyndromeTools& tool, LLRProvider& llr_provider) const = 0;
};

}

#endif // ISYNDROME_H
