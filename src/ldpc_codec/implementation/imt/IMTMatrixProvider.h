#ifndef IMTMatrixProvider_H
#define IMTMatrixProvider_H

#include "ldpc_codec/aff3ct/IAff3ctMatrixProvider.h"
#include "IMTSyndrome.h"
#include "common.h"

namespace icov {
class IMTMatrixProvider : public IAff3ctMatrixProvider
{
public:
    //FIXME
    IMTMatrixProvider(std::shared_ptr<IMTSyndromeManager> tools, std::string root):
        m_tools(tools),
        root_dir(root)
    {

    }
    ~IMTMatrixProvider() = default;


    // IMatrixProvider interface
public:
    void read_mat_imt_format();
    template<typename T>
    FastMatrix<T> read_mat_IMT_format(const std::string &filename) const;
    std::string get_alist_file(int m) const;

    void order_combine(const AffMatrix &src, const FastMatrix<uint32_t> &cmb_idx, AffMatrix &ord, AffMatrix &cmb) const;
    void order_combine(const FastMatrix<bool>& src, const FastMatrix<uint32_t> &cmb_idx, AffMatrix &ord, AffMatrix &cmb) const;

protected:
    std::string root_dir;
    std::shared_ptr<IMTSyndromeManager> m_tools;

    void _buildMatrix(int numcode, AffMatrix &matrix) override;
};
}

#endif // IMTMatrixProvider_H
