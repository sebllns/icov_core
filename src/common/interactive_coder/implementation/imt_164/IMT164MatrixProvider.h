#ifndef IMT164MatrixProvider_H
#define IMT164MatrixProvider_H

#include "interactive_coder/aff3ct/IAff3ctMatrixProvider.h"
#include "IMT164Syndrome.h"
#include "common.h"
#include <stack>

namespace icov {
class IMT164MatrixProvider : public IAff3ctMatrixProvider
{
public:
    IMT164MatrixProvider(IMT164SyndromeManager* tools, std::string root):
        m_tools(tools),
        root_dir(root)
    {

    }
    ~IMT164MatrixProvider() = default;


    // IMatrixProvider interface
public:
    AffMatrix *buildMatrix(int numcode) const override;


    std::string get_pcm(int level_idx) const;

protected:
    std::string root_dir;
    std::shared_ptr<IMT164SyndromeManager> m_tools;

};
}
#endif // IMT164MatrixProvider_H
