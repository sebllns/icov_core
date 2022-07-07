#include "Alist.h"
#include "helpers.h"
#include "Tools/Code/LDPC/AList/AList.hpp"

void icov::helpers::io::writeAlist(const std::string &filename, const AffMatrix &data)
{
    std::ofstream alist_file(filename);
    if (!alist_file.is_open())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("cannot open file " + filename));
    }

    aff3ct::tools::AList::write(data, alist_file);
}