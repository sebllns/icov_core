#ifndef IMT164SYNDROME_H
#define IMT164SYNDROME_H

#include "interactive_coder/interface/ISyndrome.h"


#include <aff3ct.hpp>
#include "interactive_coder/aff3ct/Aff3ctDecoder.h"

using namespace aff3ct;

using AffMatrix = tools::Sparse_matrix;
using index_t = uint16_t;

namespace icov {

class IMT164SyndromeManager : public ISyndromeManager
{

//struct syndrome_cell
//{
//    bool disabled;

//    icov::bit *value;
//    icov::bit *accumulation;

//    icov::bit get_value()
//    {
//        if(value == nullptr)
//        {
//            throw std::runtime_error("error syndrome cell value cannot be null");
//        }
//        icov::bit v = *value;

//        if(accumulation != nullptr)
//        {
//            v ^= *accumulation;
//        }
//        return v;
//    }
//};

public:
    std::vector<uint8_t> m_levels;


    std::vector<AffMatrix> h_dec;
    std::vector<Syndrome> synd_dec;

    std::vector<AffMatrix> h;
    std::vector<FastMatrix<index_t>> mapping_file;

    std::vector<int> h_indexes;

    icov::bit *accumulated_syndrome;//todo delete
    std::vector<std::vector<std::vector<index_t>>> m_decoded_syndrome;

    Syndrome doping_synd;

//    std::vector<syndrome_cell> syndrome_mapping;

    std::vector<size_t> doping_idx;


    bool init_rateless;

    int m_min_level;
    int m_max_level;

    int current_level;

    IMT164SyndromeManager(int n, int min_level, int max_level);
    virtual ~IMT164SyndromeManager() = default;

    virtual void init_accumulation(const std::vector<CodedVal>& source_x) override;

    virtual void compute_stored(Syndrome& o_stored_syndrome, int numcode, const LLRProvider& llr_provider) override;

    virtual void compute_deacc(Syndrome& o_syndrome, const Syndrome& acc_syndrome, int numcode, LLRProvider& llr_provider) override;

    virtual int get_m(int numcode) const override;

    void reset_doping() override;

    int get_m_max() const;

    int get_m_min() const;

    int get_m_level(int level) const;

    int get_level_count() const;

    int get_numcode(int m) const;

    int get_level(int k) const;

    int is_level(int m) const;

    void compute_doping_order(const LLRProvider& llr_provider, Syndrome &syndrome);

};
}

#include "IMT164Syndrome.hpp"
#endif // IMT164SYNDROME_H
