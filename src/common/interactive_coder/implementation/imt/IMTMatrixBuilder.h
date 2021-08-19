#ifndef IMTMATRIXBUILDER_H
#define IMTMATRIXBUILDER_H

#include "interactive_coder/aff3ct/IAff3ctMatrixBuilder.h"
#include "IMTSyndrome.h"
#include "common_types.h"

namespace icov {
class IMTMatrixBuilder : public IAff3ctMatrixBuilder
{
public:
    IMTMatrixBuilder(IMTSyndromeManager* tools, std::string root):
        m_tools(tools),
        root_dir(root)
    {

    }
    ~IMTMatrixBuilder() = default;


    // IMatrixBuilder interface
public:
    AffMatrix *buildMatrix(int numcode) const override;

    void read_mat_imt_format();
    template<typename T>
    FastMatrix<T> read_mat_IMT_format(const std::string &filename) const;
    std::string get_alist_file(int m) const;

    void order_combine(const AffMatrix &src, const FastMatrix<uint32_t> &cmb_idx, AffMatrix &ord, AffMatrix &cmb) const;
    void order_combine(const FastMatrix<bool>& src, const FastMatrix<uint32_t> &cmb_idx, AffMatrix &ord, AffMatrix &cmb) const;
protected:
    std::string root_dir;
    std::shared_ptr<IMTSyndromeManager> m_tools;
};
}
#endif // IMTMATRIXBUILDER_H
