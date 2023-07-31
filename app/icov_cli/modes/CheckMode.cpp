#include "CheckMode.h"

#include "app_common/program/icov_program_options.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "ldpc_codec_layer/io/PcmData.h"
#include <boost/exception/diagnostic_information.hpp>
#include <boost/filesystem.hpp>

using namespace icov;
using namespace icov::ldpc_codec_layer;

namespace fs = boost::filesystem;
void CheckMode::AddProgramOptions(
    boost::program_options::options_description &desc) {}

FastMatrix<bit> CheckMode::getHighLevelMatrix() {

  auto t = m_coder->syndromeMgr()->get_type();

  if (t == "imt18") {
    BOOST_THROW_EXCEPTION(std::invalid_argument(
        "Type of SyndromeManager is not supported in check mode: " + t));
  } else if (t == "imt164") {
    return getHighLevelMatrixImt164();
  } else if (t == "varodayan") {
    BOOST_THROW_EXCEPTION(std::invalid_argument(
        "Type of SyndromeManager is not supported in check mode: " + t));
  } else {
    BOOST_THROW_EXCEPTION(std::invalid_argument(
        "Type of SyndromeManager is not supported in check mode: " + t));
  }
}

icov::FastMatrix<icov::bit> CheckMode::getHighLevelMatrixImt164() {
  icov::FastMatrix<icov::bit> h;

  auto sm = m_coder->syndromeMgr();
  auto mp = m_coder->matrixProvider();

  int n = sm->get_n();
  int nrows = sm->get_m(mp->getNumcode(mp->getMatrixCount() - 1));

  fs::path path = icov::path::get_data_folder(datapath, n, true);
  path /= "H.pcm";

  std::cout << "Reading file " << path << std::endl;
  PcmData::read_matrix(path.string(), h);

  icov::assert::equal((int)h.cols(), n,
                      "Bad column count: " + std::to_string(h.cols()) +
                          " != " + std::to_string(n));

  if ((int)h.rows() != nrows) {
    BOOST_THROW_EXCEPTION(
        std::runtime_error("Bad row count: " + std::to_string(h.rows()) +
                           " != " + std::to_string(nrows)));
  }

  return h;
}

icov::FastMatrix<uint16_t> CheckMode::getMappingVector(int m) {
  icov::FastMatrix<uint16_t> v;

  assert(m > 0 && m >= m_coder->syndromeMgr()->get_m(
                           m_coder->matrixProvider()->getNumcode(0)));

  int n = m_coder->syndromeMgr()->get_n();
  int nrows = m;

  fs::path path = icov::path::get_data_folder(datapath, n, true);
  path /= "C1" + std::to_string(n / m) + ".pcm";

  std::cout << "Reading file " << path << std::endl;
  PcmData::read_mapping_vector(path.string(), v);

  if ((int)v.cols() != 2) {
    BOOST_THROW_EXCEPTION(std::runtime_error(
        "Bad column count: " + std::to_string(v.cols()) + " != 2"));
  }

  if ((int)v.rows() != nrows) {
    BOOST_THROW_EXCEPTION(
        std::runtime_error("Bad row count: " + std::to_string(v.rows()) +
                           " != " + std::to_string(nrows)));
  }

  return v;
}

int CheckMode::getNextLevel(int current) { return current / 2; }

int CheckMode::getMinLevel() {
  auto sm = m_coder->syndromeMgr();
  auto mp = m_coder->matrixProvider();
  return sm->get_m(mp->getNumcode(0));
}

void CheckMode::compare_matrix(const icov::ldpc_codec_layer::AffMatrix &m,
                               const icov::FastMatrix<icov::bit> &gt) {
  if (m.get_n_rows() != gt.cols()) {
    BOOST_THROW_EXCEPTION(std::runtime_error(
        "Row count are not equal " + std::to_string(m.get_n_rows()) +
        " != " + std::to_string(gt.cols())));
  }

  if (m.get_n_cols() != gt.rows()) {
    BOOST_THROW_EXCEPTION(std::runtime_error(
        "Columns count are not equal " + std::to_string(m.get_n_cols()) +
        " != " + std::to_string(gt.rows())));
  }

  for (int i = 0; i < (int)gt.rows(); i++) {
    for (int j = 0; j < (int)gt.cols(); j++) {
      if (gt.at(i, j) != m.at(j, i)) {
        BOOST_THROW_EXCEPTION(std::runtime_error("Matrices are not equal at " +
                                                 std::to_string(i) + ", " +
                                                 std::to_string(j)));
      }
    }
  }
}

CheckMode::CheckMode(const po::variables_map &args,
                     const std::string &data_path)
    : m_coder(icov::options::create_coder(args)),
      m_stored_syndrome(m_coder->syndromeMgr()->get_n()), datapath(data_path) {}

void CheckMode::run() {
  auto mp = m_coder->matrixProvider();
  auto sm = m_coder->syndromeMgr();
  auto lp = m_coder->llrProvider();

  // read initial matrix
  auto m_src = getHighLevelMatrix();

  // read acc vector
  auto v_acc = getMappingVector(getNextLevel(m_src.rows()));

  // get next level
  int j = 0;
  int i = mp->getMatrixCount();

  icov::Syndrome stored_syndrome(sm->get_n());
  icov::Syndrome decoded_syndrome(sm->get_n());

  while ((int)m_src.rows() > sm->get_m(mp->getNumcode(0))) {
    auto m = mp->getMatrix(--i);
    int numcode = mp->getNumcode(i);

    // fill matrix
    int row_count = sm->get_m(numcode);
    int col_count = sm->get_n();

    std::cout << "Performing check for m=" << row_count << std::endl;

    icov::FastMatrix<icov::bit> m_gt(row_count, col_count);

    j = m_src.rows() - row_count;

    // compute gt
    std::vector<int> deleted(m_src.rows(), 0);

    for (int c = 0; c < col_count; c++) {
      for (int k = 0; k < j; k++) {
        uint16_t r1 = v_acc.at(k, 0);
        uint16_t r2 = v_acc.at(k, 1);

        if (r1 >= (uint16_t)deleted.size() || r2 >= (uint16_t)deleted.size()) {
          BOOST_THROW_EXCEPTION(
              std::runtime_error("Overflow in accumulation vector"));
        }
        deleted[r1] = 1;
        deleted[r2] = 1;

        // acc r1,r2
        m_gt.at(k, c) = m_src.at(r1, c) ^ m_src.at(r2, c);
      }

      int l = j;
      for (size_t k = 0; k < m_src.rows(); k++) {
        if (!deleted[k]) {
          m_gt.at(l++, c) = m_src.at(k, c);
        }
      }
    }

    // compare gt
    try {
      compare_matrix(m, m_gt);
    } catch (...) {
      ICOV_ERROR << "Error: matrix check failed for m=" << row_count
                 << std::endl;
      ICOV_ERROR << boost::current_exception_diagnostic_information()
                 << std::endl;
    }

    if (j == (int)v_acc.rows()) {
      m_src = m_gt;
      if ((int)m_src.rows() > sm->get_m(mp->getNumcode(0)))
        v_acc = getMappingVector(getNextLevel(m_src.rows()));
    }

    auto src = lp->initSrcRand(m_coder->get_n());
    stored_syndrome.resize(0);
    decoded_syndrome.resize(0);

    sm->init_accumulation(src);
    sm->compute_stored(stored_syndrome, numcode, *lp, src);
    if ((int)stored_syndrome.size() != row_count) {
      BOOST_THROW_EXCEPTION(
          std::runtime_error("Stored syndrome bad size: " +
                             std::to_string(stored_syndrome.size()) +
                             " != " + std::to_string(row_count)));
    }

    sm->compute_deacc(decoded_syndrome, stored_syndrome, numcode, *lp);
    if ((int)decoded_syndrome.size() != row_count) {
      BOOST_THROW_EXCEPTION(
          std::runtime_error("Decoded syndrome bad size: " +
                             std::to_string(decoded_syndrome.size()) +
                             " != " + std::to_string(row_count)));
    }

    for (int k = 0; k < row_count; k++) {
      icov::bit s = 0;

      for (int ki = 0; ki < col_count; ki++) {
        s ^= src[ki] * m_gt.at(k, ki);
      }

      if (s != decoded_syndrome[k]) {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "Error detected in syndrome computation at index " +
            std::to_string(k)));
      }
    }
  }
}
