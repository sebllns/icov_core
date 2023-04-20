#include "IMT164MatrixProvider.h"

#include "io/PcmData.h"
#include "tools/DebugTools.h"
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <cstdint>
#include <fstream>
#include <sstream>

namespace icov {
namespace ldpc_codec_layer {

IMT164MatrixProvider::IMT164MatrixProvider(const std::string &root,
                                           int min_level, int max_level,
                                           const IMT164SyndromeTools &tools)
    : root_dir(root), current_level(max_level), m_min_level(min_level),
      m_max_level(max_level), m_tools(tools) {
  int l = m_min_level;
  while (l <= m_max_level) {
    m_levels.insert(m_levels.begin(), l);
    l *= 2;
  }

  this->h.resize(get_level_count());
  this->mapping_file.resize(get_level_count() - 1, FastMatrix<index_t>(0, 0));
}

void IMT164MatrixProvider::buildHighMatrix(int level_idx, AffMatrix &H) {
  ICOV_TRACE << "build high matrix";
  this->current_level = level_idx;

  PcmData::read_matrix(get_pcm_path(level_idx), H);
  PcmData::read_mapping_vector(get_pcm_path(level_idx - 1),
                               this->mapping_file[level_idx - 1]);

  this->h_indexes = std::vector<int>(H.get_n_cols());
  std::iota(this->h_indexes.begin(), this->h_indexes.end(), 0);

  this->h_dec.insert(this->h_dec.begin(), H);
  this->h[level_idx] = H;
}

void IMT164MatrixProvider::_buildMatrix(int numcode, AffMatrix &H) {
  int m = m_tools.get_m(numcode);
  int n = m_tools.get_n();

  ICOV_DEBUG << "Build matrix " << m << " x " << n;

  int level_idx = this->is_level(m);

  if (level_idx == this->get_level_count() - 1) {
    // build matrix m = n/2
    H = AffMatrix(n, m);
    buildHighMatrix(level_idx, H);
  } else {

    accumulateRows(numcode, H);

    if (level_idx > 0) {
      PcmData::read_mapping_vector(get_pcm_path(level_idx - 1),
                                   this->mapping_file[level_idx - 1]);
    }

    if (level_idx >= 0) {
      this->current_level = level_idx;
      this->h[level_idx] = H;

      this->h_indexes = std::vector<int>(H.get_n_cols());
      std::iota(this->h_indexes.begin(), this->h_indexes.end(), 0);
    }

    this->h_dec.insert(this->h_dec.begin(), H);

    //        AffMatrix previous_mat =  this->h_dec[0];

    //        if((int)previous_mat.get_n_cols() != (m+1))
    //        {
    //            _buildMatrix(numcode+1, previous_mat);
    //        }

    // accumulateRows(numcode, level_idx, H, previous_mat);

    //        AffMatrix &previous_mat = this->h_dec[0];
    //        int row_count = previous_mat.get_n_cols() - m;

    //        while ( row_count > 0)
    //        {
    //            accumulateRows(numcode, level_idx, H, this->h_dec[0]);
    //            previous_mat = this->h_dec[0];
    //            row_count--;
    //        }

    // build matrix m
    if (m == m_tools.get_m_min()) {
      computeSyndromeCacheIndex();
    }
  }

  ICOV_TRACE << "IMT164 " << numcode << ": " << m << " x " << n << " -> "
             << H.get_n_cols() << " x " << H.get_n_rows();
}

void IMT164MatrixProvider::accumulateRows(int numcode, AffMatrix &dst) {
  int m = m_tools.get_m(numcode);

  int level_idx = 1;

  while (level_idx < (this->get_level_count() - 1) &&
         this->get_m_level(this->get_level(level_idx)) <= m) {
    level_idx++;
  }

  auto &src = this->h[level_idx];

  m_deleted_row.resize(src.get_n_cols());
  m_deleted_row.fill(false);

  int row_idx = src.get_n_cols() - m;
  dst = AffMatrix(src.get_n_rows(), m);

  for (int i = 0; i < (int)dst.get_n_rows(); i++) {
    for (int j = 0; j < row_idx; j++) {
      int row_src1 = this->mapping_file[level_idx - 1].at(j, 0);
      int row_src2 = this->mapping_file[level_idx - 1].at(j, 1);

      if (src.at(i, row_src1) ^ src.at(i, row_src2)) {
        dst.add_connection(i, j);
      }

      // deleted_row
      m_deleted_row[row_src1] = true;
      m_deleted_row[row_src2] = true;
    }

    int k = row_idx;
    for (int j = 0; j < (int)src.get_n_cols(); j++) {
      if (!m_deleted_row[j]) {
        if (src.at(i, j))
          dst.add_connection(i, k);
        k++;
      }
    }

    if (k != m) {
      throw std::runtime_error("k != m");
    }
  }
}

void IMT164MatrixProvider::accumulateRows(int numcode, int level_idx,
                                          AffMatrix &dst,
                                          const AffMatrix &src) {
  dst = AffMatrix(m_tools.get_n(), m_tools.get_m(numcode));

  int row_count = this->get_m_level(this->get_level(this->current_level)) -
                  m_tools.get_m(numcode);
  // std::cout << this->current_level << ", " << row_count << std::endl;

  int row_dst = row_count - 1;
  auto &indexes = this->h_indexes;

  int row_src1 = this->mapping_file[this->current_level - 1].at(row_dst, 0);
  int row_src2 = this->mapping_file[this->current_level - 1].at(row_dst, 1);

  std::vector<int> indexes2;

  int row1_idx = 0;
  int row2_idx = 0;

  for (int k = 0; k < (int)indexes.size(); k++) {
    if (indexes[k] == row_src1) {
      row1_idx = k + row_dst;
    } else if (indexes[k] == row_src2) {
      row2_idx = k + row_dst;
    } else {
      indexes2.push_back(indexes[k]);
    }
  }

  for (int j = 0; j < (int)dst.get_n_rows(); j++) {
    int offset = 0;

    for (int i = 0; i < (int)dst.get_n_cols(); i++) {

      if (i == row_dst) {

        // src = this->h_dec[0]
        if (src.at(j, row1_idx) ^ src.at(j, row2_idx)) {
          dst.add_connection(j, i);
        }

        offset--;
      } else {
        if (i < row_dst) {
          offset = 0;
        } else if ((i + offset) == row1_idx || (i + offset) == row2_idx) {
          offset++;
        }

        if (src.at(j, i + offset)) {
          dst.add_connection(j, i);
        }
      }
    }
  }

  indexes = indexes2;
}

void IMT164MatrixProvider::computeSyndromeCacheIndex() {
  std::vector<std::vector<index_t>> syndrome_index;

  for (int k = 0; k < this->get_level_count(); k++) {
    int l = this->get_level(k);

    int m_min = ((k == 0) ? (this->get_m_level(l))
                          : (this->get_m_level(this->get_level(k - 1)) + 1));

    int m_max = this->get_m_level(l) + 1;

    for (int ml = m_min; ml < m_max; ml++) {
      if (k == 0) {
        syndrome_index.resize(ml);
        for (int i = 0; i < ml; i++) {
          syndrome_index[i] = std::vector<index_t>(1, i + 1);
        }
      } else {
        int m_diff = ml - (m_min - 1);
        int m_previous = ml - 2 * m_diff;

        auto erased = syndrome_index[m_previous];
        syndrome_index.erase(syndrome_index.begin() + m_previous);

        // C2
        erased.push_back(ml);

        // C1
        auto inserted = std::vector<index_t>(1, ml);

        // m_max-m_min
        int crow = m_max - ml - 1;
        index_t c1 = this->mapping_file[k - 1].at(crow, 0);
        index_t c2 = this->mapping_file[k - 1].at(crow, 1);

        index_t c1pos = syndrome_index.size();
        index_t c2pos = c1pos + 1;

        // c1pos < c2pos ? c2pos++ : c1pos++;

        for (int c = m_max - ml - 1; c < m_max - m_min; c++) {
          index_t d1 = this->mapping_file[k - 1].at(c, 0);
          index_t d2 = this->mapping_file[k - 1].at(c, 1);
          if (c1 < d1)
            c1pos--;
          if (c1 < d2 && d2 != c2)
            c1pos--;
          if (c2 < d1)
            c2pos--;
          if (c2 < d2)
            c2pos--;
        }

        syndrome_index.insert(syndrome_index.begin() + c1pos, inserted);
        syndrome_index.insert(syndrome_index.begin() + c2pos, erased);

        // std::cout << syndrome_index.size() << std::endl;
      }

      this->m_decoded_syndrome.push_back(syndrome_index);
    }
  }
}

///
/// \brief Get the original matrix level that the given matrix height is
/// computed from.
///
/// \param m Height of decoding matrix
///
/// \return Level of the
/// source file for accumulation
///
int IMT164MatrixProvider::is_level(int m) const {
  for (int i = 0; i < get_level_count(); ++i) {
    if (get_m_level(get_level(i)) == m) {
      return i;
    }
  }
  return -1;
}

int IMT164MatrixProvider::get_m_level(int level) const {
  return m_tools.get_n() / level;
}

int IMT164MatrixProvider::get_level_count() const { return m_levels.size(); }

int IMT164MatrixProvider::get_level(int k) const { return m_levels[k]; }

int IMT164MatrixProvider::min_level() const { return m_min_level; }

int IMT164MatrixProvider::max_level() const { return m_max_level; }

const std::vector<index_t> &
IMT164MatrixProvider::decode_syndrome_indexes(int numcode, int idx) const {
  return this->m_decoded_syndrome[std::min(
      numcode, (int)m_decoded_syndrome.size() - 1)][idx];
}

index_t IMT164MatrixProvider::mapping_file_index(int i, int j, int k) const {
  return mapping_file[i].at(j, k);
}

const std::vector<aff3ct::tools::Sparse_matrix::Idx_t> &
IMT164MatrixProvider::get_decoding_matrix_col(int h_idx, int col_idx) const {
  return h[h_idx].get_rows_from_col(col_idx);
}

std::string IMT164MatrixProvider::get_matrix_root(std::string root_dir,
                                                  int coder_length) {
  return (boost::filesystem::path(root_dir) / std::to_string(coder_length))
      .string();
}

std::string IMT164MatrixProvider::get_pcm_path(int level_idx) const {
  std::string filename = "H";
  if (level_idx < this->get_level_count() - 1) {
    filename = "C1" + std::to_string(this->get_level(level_idx));
  }
  // get data dir
  return (boost::filesystem::path(get_matrix_root(root_dir, m_tools.get_n())) /
          (filename + ".pcm"))
      .string();
}
} // namespace ldpc_codec_layer
} // namespace icov
