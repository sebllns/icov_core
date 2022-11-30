#include "Alist.h"
#include "Tools/Code/LDPC/AList/AList.hpp"
#include "helpers.h"

void icov::helpers::io::writeAlist(const std::string &filename,
                                   const AffMatrix &data) {
  std::ofstream alist_file(filename);
  icov::helpers::assert::check(alist_file.is_open(),
                               "Cannot open file " + filename);
  aff3ct::tools::AList::write(data, alist_file);
}
