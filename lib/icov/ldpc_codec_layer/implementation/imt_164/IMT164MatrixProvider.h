#ifndef IMT164MatrixProvider_H
#define IMT164MatrixProvider_H

#include "aff3ct/IAff3ctMatrixProvider.h"
#include "IMT164Syndrome.h"
#include "common.h"
#include <stack>

namespace icov {
class IMT164MatrixProvider : public IAff3ctMatrixProvider
{
public:
    IMT164MatrixProvider(std::shared_ptr<IMT164SyndromeManager> tools, std::string root):
        m_tools(tools),
        root_dir(root)
    {

    }
    ~IMT164MatrixProvider() = default;


    // IMatrixProvider interface
public:
    std::string get_pcm_path(int level_idx) const;

protected:
    std::string root_dir;
    std::shared_ptr<IMT164SyndromeManager> m_tools;
    icov::FastArray<bool> m_deleted_row;

    void _buildMatrix(int numcode, AffMatrix &H) override;

    void buildHighMatrix(int level_idx, AffMatrix &H);
    void accumulateRows(int numcode, AffMatrix &dst);
    void accumulateRows(int numcode, int level_idx, AffMatrix &dst, const AffMatrix &src);
    void computeSyndromeCacheIndex();
};
}
#endif // IMT164MatrixProvider_H
