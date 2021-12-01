#include "CheckMode.h"

#include "interactive_coder/factory/CoderFactory.h"
#include "tools/DebugTools.h"
#include "io/PcmData.h"
#include <boost/exception/diagnostic_information.hpp>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;
void CheckMode::AddProgramOptions(boost::program_options::options_description &desc)
{
    desc.add_options()
            (
                "check-iterations",
                po::value<int>()->default_value(100),
                "set iteration count for test loop"
             );
}

FastMatrix<bit> CheckMode::getHighLevelMatrix()
{

    auto t = m_coder->syndromeMgr()->get_type();

    if(t == "imt18")
    {
        BOOST_THROW_EXCEPTION(std::invalid_argument("Type of SyndromeManager is not supported in check mode: " + t));
    }
    else if(t == "imt164")
    {
        return getHighLevelMatrixImt164();
    }
    else if(t == "varodayan")
    {
        BOOST_THROW_EXCEPTION(std::invalid_argument("Type of SyndromeManager is not supported in check mode: " + t));
    }
    else
    {
         BOOST_THROW_EXCEPTION(std::invalid_argument("Type of SyndromeManager is not supported in check mode: " + t));
    }
}

FastMatrix<bit> CheckMode::getHighLevelMatrixImt164()
{
    FastMatrix<icov::bit> h;

    auto sm = m_coder->syndromeMgr();
    auto mp = m_coder->matrixProvider();

    int n =  sm->get_n();
    int nrows = sm->get_m(mp->getNumcode(mp->getMatrixCount()-1));

    fs::path path = icov::helpers::path::get_data_folder(datapath, n, true);
    path /= "H.pcm";

    std::cout << "Reading file " << path << std::endl;
    PcmData::read_matrix(path.string(), h);

    if((int)h.cols() != n)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Bad column count: " + std::to_string(h.cols())  + " != " + std::to_string(n)));
    }


    if((int)h.rows() != nrows)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Bad row count: " + std::to_string(h.rows())  + " != " + std::to_string(nrows)));
    }

    return h;
}

FastMatrix<uint16_t> CheckMode::getMappingVector(int m)
{
    FastMatrix<uint16_t> v;

    int n =  m_coder->syndromeMgr()->get_n();
    int nrows = m;

    fs::path path = icov::helpers::path::get_data_folder(datapath, n, true);
    path /= "C1" + std::to_string(n/m) + ".pcm";

    std::cout << "Reading file " << path << std::endl;
    PcmData::read_mapping_vector(path.string(), v);

    if((int)v.cols() != 2)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Bad column count: " + std::to_string(v.cols())  + " != 2"));
    }

    if((int)v.rows() != nrows)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Bad row count: " + std::to_string(v.rows())  + " != " + std::to_string(nrows)));
    }


    std::cout << "BEFORE RETURN\n" << v.to_string() << std::endl;

    return v;
}

int CheckMode::getNextLevel(int current)
{
    return current / 2;
}

int CheckMode::getMinLevel()
{
    auto sm = m_coder->syndromeMgr();
    auto mp = m_coder->matrixProvider();
    return sm->get_m(mp->getNumcode(0));
}

void CheckMode::compare_matrix(const AffMatrix &m, const  icov::FastMatrix<icov::bit> &gt)
{
    if(m.get_n_rows() != gt.cols())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Row count are not equal " + std::to_string(m.get_n_rows()) + " != " + std::to_string(gt.cols())));
    }

    if(m.get_n_cols() != gt.rows())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Columns count are not equal " + std::to_string(m.get_n_cols()) + " != " + std::to_string(gt.rows())));
    }

    for(int i = 0 ; i < (int)gt.rows() ; i++)
    {
        for(int j = 0 ; j < (int)gt.cols() ; j++)
        {
            if(gt.at(i,j) != m.at(j,i))
            {
                BOOST_THROW_EXCEPTION(std::runtime_error("Matrices are not equal at " + std::to_string(i) + ", " + std::to_string(j)));
            }
        }
    }
}

CheckMode::CheckMode(const po::variables_map &args, const std::string& data_path):
    m_coder(CoderFactory::create_coder(args)),
    m_stored_syndrome(m_coder->syndromeMgr()->get_n()),
    datapath(data_path)
{
}

void CheckMode::run()
{
    auto mp = m_coder->matrixProvider();
    auto sm = m_coder->syndromeMgr();

    //read initial matrix
    auto m_src = getHighLevelMatrix();

    //read acc vector
    auto v_acc = getMappingVector(getNextLevel(m_src.rows()));

    //get next level
    int j = 0;
    int i = mp->getMatrixCount();

    while((int)m_src.rows() > sm->get_m(mp->getNumcode(0)))
    {        
        auto m = mp->getMatrix(--i);
        int numcode = mp->getNumcode(i);

        //fill matrix
        int row_count  = sm->get_m(numcode);
        int col_count = sm->get_n();

        std::cout << "Performing check for m=" << row_count << std::endl;

        icov::FastMatrix<icov::bit> m_gt (row_count, col_count);

        j = m_src.rows() - row_count;

        //compute gt
        std::vector<int> deleted(m_src.rows(), 0);


        for(int c = 0; c < col_count ; c++)
        {
            for(int k = 0; k < j ; k++)
            {
                uint16_t r1 = v_acc.at(k, 0);
                uint16_t r2 = v_acc.at(k, 1);

                if(r1 >= (uint16_t)deleted.size() || r2 >= (uint16_t)deleted.size())
                {
                    BOOST_THROW_EXCEPTION(std::runtime_error("Overflow in accumulation vector"));
                }
                deleted[r1] = 1;
                deleted[r2] = 1;

                //acc r1,r2
                m_gt.at(k, c) = m_src.at(r1, c) ^ m_src.at(r2, c);
            }

            int l = j;
            for(int k = 0; k < m_src.rows() ; k++)
            {
                if(!deleted[k])
                {
                    m_gt.at(l++, c) = m_src.at(k, c);
                }
            }
        }

        //compare gt
        try
        {
            compare_matrix(m, m_gt);
        }  catch (...)
        {
            std::cerr << "Error: matrix check failed for m=" << row_count << std::endl;
            std::cerr << boost::current_exception_diagnostic_information() << std::endl;
        }


        if(j == v_acc.rows())
        {
            m_src = m_gt;
            if(m_src.rows() > sm->get_m(mp->getNumcode(0))) v_acc = getMappingVector(getNextLevel(m_src.rows()));
            j = 0;
        }

        //sm->compute_stored()
        //sm->compute_deacc()
    }
}
