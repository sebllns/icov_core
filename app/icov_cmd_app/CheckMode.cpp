#include "CheckMode.h"

#include "interactive_coder/factory/CoderFactory.h"
#include "tools/DebugTools.h"

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>

using namespace boost::numeric::ublas;

void CheckMode::AddProgramOptions(boost::program_options::options_description &desc)
{
    desc.add_options()
            (
                "Check-iterations",
                po::value<int>()->default_value(100),
                "set iteration count for test loop"
             );
}

CheckMode::CheckMode(const po::variables_map &args):
    m_coder(CoderFactory::create_coder(args)),
    m_src_bits(m_coder->syndromeMgr()->get_n()),
    m_dec_bits(m_coder->syndromeMgr()->get_n()),
    m_stored_syndrome(m_coder->syndromeMgr()->get_n()),
    m_numcodeIdx(0),
    m_error_count(0)
{
    m_nite = args["Check-iterations"].as<int>();
}

void CheckMode::run()
{
    std::cout<<"encode\n";

    auto mp = m_coder->matrixProvider();
    auto sm = m_coder->syndromeMgr();

    for (int i = 0; i < mp->getMatrixCount(); i++)
    {
        auto m = mp->getMatrix(i);

        //fill matrix
        matrix<icov::bit> m_gt (3, 3);

        //compare gt

        //sm->compute_stored()
        //sm->compute_deacc()


    }


}
