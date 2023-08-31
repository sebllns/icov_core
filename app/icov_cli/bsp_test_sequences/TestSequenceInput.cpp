#include "TestSequenceInput.h"
#include <boost/algorithm/string/trim.hpp>

namespace icov
{
TestSequenceInput::TestSequenceInput(size_t size):
    m_size(size)
{

}

bool TestSequenceInput::read_from_file(std::string filename)
{
    m_data.clear();

    std::ifstream file(filename);
    std::string line;

    if(!file.is_open()) throw std::runtime_error("cannot open file " + filename);

    while (std::getline(file, line))
    {
        boost::algorithm::trim_left(line);
        if(line[0] == '#')
        {
            //comment: continue
            continue;
        }

        std::vector<data_type> data(m_size, 0);

        data_type val;
        std::istringstream line_stream(line);
        for(size_t i = 0; i < data.size(); i++)
        {
            line_stream  >> val;
            data[i] = val;
            if (line_stream.peek() == ',') line_stream.ignore();
        }

        m_data.push_back(data);
    }

    return true;
}

const std::vector<TestSequenceInput::data_type> &TestSequenceInput::get_source() const
{
    return m_data[0];
}

const std::vector<TestSequenceInput::data_type> &TestSequenceInput::get_side_info(int idx) const
{
    return m_data[idx+1];
}

int TestSequenceInput::get_side_info_count() const
{
    return m_data.size()-1;
}
} // namespace icov
