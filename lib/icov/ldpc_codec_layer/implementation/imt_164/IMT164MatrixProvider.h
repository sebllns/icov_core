#pragma once

#include "icov_common.h"
#include "ldpc_codec_layer/aff3ct/IAff3ctMatrixProvider.h"
#include "ldpc_codec_layer/implementation/imt_164/IMT164SyndromeTools.h"
#include <stack>

namespace icov {
namespace ldpc_codec_layer {

class IMT164MatrixProvider : public IAff3ctMatrixProvider {
public:
  IMT164MatrixProvider(const std::string &root, int min_level, int max_level,
                       const IMT164SyndromeTools &tools);
  ~IMT164MatrixProvider() = default;

  // IMatrixProvider interface
  static std::string get_matrix_root(std::string root_dir, int coder_length);
  std::string get_pcm_path(int level_idx) const;

  int get_m_level(int level) const;
  int get_level_count() const;
  int get_level(int k) const;
  int is_level(int m) const;
  int min_level() const;
  int max_level() const;
  const std::vector<index_t> &decode_syndrome_indexes(int numcode,
                                                      int idx) const;

  index_t mapping_file_index(int i, int j, int k) const;
  const std::vector<AffMatrix::Idx_t> &
  get_decoding_matrix_col(int h_idx, int col_idx) const;

protected:
  void _buildMatrix(int numcode, AffMatrix &H) override;

  void buildHighMatrix(int level_idx, AffMatrix &H);
  void accumulateRows(int numcode, AffMatrix &dst);
  void accumulateRows(int numcode, int level_idx, AffMatrix &dst,
                      const AffMatrix &src);
  void computeSyndromeCacheIndex();

private:
  std::string root_dir;
  icov::FastArray<bool> m_deleted_row;

  std::vector<AffMatrix> h_dec;
  std::vector<int> h_indexes;
  int current_level;
  std::vector<AffMatrix> h;
  std::vector<FastMatrix<index_t>> mapping_file;
  std::vector<std::vector<std::vector<index_t>>> m_decoded_syndrome;

  std::vector<int8b> m_levels;
  int m_min_level;
  int m_max_level;

  IMT164SyndromeTools m_tools;
};
} // namespace ldpc_codec_layer
} // namespace icov
