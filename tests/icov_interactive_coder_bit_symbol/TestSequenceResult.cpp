#include "TestSequenceResult.h"
#include <boost/algorithm/string/trim.hpp>

TestSequenceResult::TestSequenceResult()
{

}

bool TestSequenceResult::read_from_file(string filename)
{
    m_data.clear();

    std::ifstream file(filename);
    std::string line;

    if(!file.is_open()) throw std::runtime_error("cannot open file " + filename);

    result_data result;

    bool read_data = false;

    while (std::getline(file, line))
    {

        boost::algorithm::trim_left(line);
        if(line[0] == '#')
        {
            //parse comment
            std::size_t found = line.rfind("name:");
            if(found!=std::string::npos)
            {
                result.name = boost::algorithm::trim_copy(line.substr(line.rfind(":") + 1));
                result.data.clear();
                result.n_rows = 1;
                result.n_cols = 1;
                read_data = false;
            }

            found = line.rfind("type:");
            if(found!=std::string::npos)
            {
                std::string type_str = boost::algorithm::trim_copy(line.substr(line.rfind(":") + 1));

                if(type_str == "matrix")
                {
                    result.type = result_data_type::MATRIX;
                }
                else if(type_str == "scalar")
                {
                    result.type = result_data_type::SCALAR;
                    read_data = true;
                }
            }

            if(result.type == result_data_type::MATRIX)
            {
                found = line.rfind("rows:");
                if(found!=std::string::npos)
                {
                    std::stringstream row_stream(boost::algorithm::trim_copy(line.substr(line.rfind(":") + 1)));
                    row_stream >> result.n_rows;
                }

                found = line.rfind("columns:");
                if(found!=std::string::npos)
                {
                    std::stringstream col_stream(boost::algorithm::trim_copy(line.substr(line.rfind(":") + 1)));
                    col_stream >> result.n_cols;
                    read_data = true;
                }
            }
        }
        else if(read_data)
        {
            std::stringstream line_stream(line);
            float val = 0.0;
            for(int i = 0; i < result.n_cols; i++)
            {
                line_stream  >> val;
                result.data.push_back(val);
            }

            if(result.data.size() == (size_t)(result.n_cols * result.n_rows))
            {
                read_data = false;
                m_data[result.name] = result;
            }
        }
    }

    return true;
}
