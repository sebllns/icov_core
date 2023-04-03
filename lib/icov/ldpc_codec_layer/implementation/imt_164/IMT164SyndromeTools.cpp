#include "IMT164SyndromeTools.h"

using namespace icov;

IMT164SyndromeTools::IMT164SyndromeTools(int n, int min_m, int max_m)
    : m_n(n), m_min_m(min_m), m_max_m(max_m) {}

int IMT164SyndromeTools::get_m_max() const { return m_max_m; }

int IMT164SyndromeTools::get_m_min() const { return m_min_m; }

int IMT164SyndromeTools::get_numcode(int m) const {
  return m - this->get_m_min();
}

int IMT164SyndromeTools::get_n() const { return m_n; }

///
/// \brief Get the height of the decoding matrix and length of associated
/// syndrome from a given numcode index
///
/// \param numcode index
///
/// \return Length of associated syndrome that is also the height of the
/// decoding matrix
///
int IMT164SyndromeTools::get_m(int numcode) const {
  // m is given by the numcode with an offset of n/8 (minimum rate)
  return this->get_m_min() + numcode;
}
