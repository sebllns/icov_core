#include "SymboleMode.h"

#include "TestSequenceInput.h"
#include "TestSequenceResult.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "tools/DebugTools.h"

#include <boost/algorithm/string.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/filesystem.hpp>
#include <boost/throw_exception.hpp>

#include <fstream>
#include <stdexcept>

namespace fs = boost::filesystem;

// estimate probability and rate
//    std::vector<std::pair<float,float>> sim_result;
// float estimated_rate = estimate_rate(b.src_bits[i], i < ly ? b.si_bits[i] :
// zero_vec, b.sim_result);

// float estimate_rate(const BitVec &src, const BitVec &si,
//                    const std::vector<std::pair<float, float>> &sim_result) {
//  //    for(int i = 0 ; i < src.size() ; i++)
//  //    {
//  //        std::cout << src[i] << ", ";
//  //    }

//  //    std::cout << std::endl;

//  //    for(int i = 0 ; i < si.size() ; i++)
//  //    {
//  //        std::cout << si[i] << ", ";
//  //    }

//  std::cout << std::endl;

//  float r = 0.0;
//  float p = 0.0;

//  int lx = src.size();
//  int ly = si.size();

//  p = std::abs(lx - ly);

//  for (int i = 0; i < std::min(lx, ly); i++) {
//    if (src[i] != si[i]) {
//      p++;
//    }
//  }

//  p /= src.size();

//  // std::cout << "p error = " << p << std::endl;

//  int i = 0;
//  while (i < (int)sim_result.size() && sim_result[i].first < p) {
//    i++;
//  }

//  // todo interpolation ?
//  r = sim_result[i].second;

//  std::cout << "estimated rate=" << r << std::endl;

//  return r;
//}

void SymboleMode::AddProgramOptions(
    boost::program_options::options_description &desc) {
  desc.add_options()(dataset_dir_id.c_str(), po::value<std::string>(),
                     "Input directory")(reference_dir_id.c_str(),
                                        po::value<std::string>(),
                                        "Reference directoty")(
      result_dir_id.c_str(), po::value<std::string>(), "Result directoty")(
      max_si_count_id.c_str(), po::value<int>()->default_value(8),
      "Side info count")(enable_q_quantization_id.c_str(),
                         po::value<bool>()->default_value(true),
                         "Enable quantization of parameter q from values in "
                         "file <data-path>/../q-ary/q")(
      enable_z_quantization_id.c_str(), po::value<bool>()->default_value(true),
      "Enable quantization of parameters zmin, zmax");
}

SymboleMode::SymboleMode(const boost::program_options::variables_map &args)
    : m_coder(CoderFactory::create_coder(args)) {

  icov::helpers::assert::check(args.count(dataset_dir_id),
                               dataset_dir_id + " is missing");

  icov::helpers::assert::check(args.count(result_dir_id),
                               result_dir_id + " is missing");

  icov::helpers::assert::check(args.count(reference_dir_id),
                               reference_dir_id + " is missing");

  m_dataset_dir =
      fs::absolute(args[dataset_dir_id].as<std::string>()).make_preferred();
  m_references_dir =
      m_dataset_dir / "results" / args[reference_dir_id].as<std::string>();
  m_result_dir =
      fs::absolute(args[result_dir_id].as<std::string>()).make_preferred();
  max_si_count = args[max_si_count_id].as<int>();
  enable_q_quantization = args[enable_q_quantization_id].as<bool>();
  enable_z_quantization = args[enable_z_quantization_id].as<bool>();

  if (enable_q_quantization) {
    icov::helpers::io::read_vector(ICOV_Q_VALUES_FILE.string(), q_values);
  }

  helpers::path::assert_directory_exists(m_dataset_dir.string());
  helpers::path::assert_directory_exists(m_references_dir.string());
  helpers::path::try_create_directories(m_result_dir.string());
  helpers::path::assert_directory_exists(m_result_dir.string());
}

void SymboleMode::run() {
  auto measure_path = fs::path(m_result_dir) / "metric.csv";
  measure_file = std::ofstream(measure_path.string());
  measure_file << "file\tsi\tabs diff\trelative diff\n";

  icov::helpers::assert::check(measure_file.is_open(),
                               "cannot open measure file for " +
                                   measure_path.string());

  fs::path seq_list = fs::path(m_dataset_dir) / "files.txt";
  process_input_list(seq_list.string());
  measure_file.close();
}

void SymboleMode::write_results(const std::string &sequence_name,
                                const std::vector<NumcodeIdxVec> &numcodes) {
  TestSequenceResult tsr;
  tsr.read_from_file((m_references_dir / (sequence_name + ".mat")).string());

  std::ofstream result_file(
      (fs::path(m_result_dir) / (sequence_name + ".txt")).string());

  icov::helpers::assert::check(result_file.is_open(),
                               "cannot open result file for " + sequence_name);

  result_file << "# ICOV rate" << std::endl;
  for (auto &si : numcodes) {
    for (auto &bp_numcode : si) {
      result_file << std::fixed << std::setw(7) << std::setprecision(6)
                  << std::setfill('0') << m_coder->get_r(bp_numcode) << " ";
    }
    result_file << std::endl;
  }
  result_file << std::endl;

  result_file << "#Ground truth" << std::endl;
  auto &frangping_rate = tsr["R"];
  for (int i = 0; i < std::min(max_si_count, frangping_rate.n_rows); i++) {
    for (int j = 0; j < frangping_rate.n_cols; j++) {
      result_file << std::fixed << std::setw(7) << std::setprecision(6)
                  << std::setfill('0')
                  << frangping_rate.get_data(i, frangping_rate.n_cols - 1 - j)
                  << " ";
    }
    result_file << std::endl;
  }

  result_file << std::endl;

  result_file.close();

  auto syndromeMgr = m_coder->syndromeMgr();
  float min_rate = (1.0 * syndromeMgr->get_m(0)) / syndromeMgr->get_n();
  for (size_t si = 0; si < numcodes.size(); si++) {
    float icov_rate = 0.0, fp_rate = 0.0;
    for (size_t bp = 0; bp < numcodes[si].size(); bp++) {
      icov_rate += m_coder->get_r(numcodes[si][bp]);
      fp_rate += std::max(tsr["R"].get_data(si, frangping_rate.n_cols - 1 - bp),
                          min_rate);
    }

    measure_file << sequence_name << "\t" << si + 1 << "\t" << std::fixed
                 << std::setw(7) << std::setprecision(6) << std::setfill('0')
                 << (icov_rate - fp_rate) << "\t"
                 << (icov_rate - fp_rate) / fp_rate << std::endl;
  }

  measure_file.flush();
}

void SymboleMode::process_input_list(const std::string &file) {
  std::ifstream seq_list(file);

  icov::helpers::assert::check(seq_list.is_open(), "cannot open file " + file);

  std::string sequence_name;
  while (std::getline(seq_list, sequence_name)) {
    boost::algorithm::trim(sequence_name);
    if (!sequence_name.empty()) {
      try {
        fs::path seq_path =
            fs::path(m_dataset_dir) / "sequences" / (sequence_name + ".txt");

        ICOV_INFO << "Process file " << seq_path;
        process_input_file(seq_path.string());
      } catch (...) {
        ICOV_ERROR << "Error: Could not process sequence file "
                        << sequence_name;
        ICOV_ERROR << boost::current_exception_diagnostic_information();
      }
    }
  }
  seq_list.close();
}

void SymboleMode::process_input_file(const std::string &file) {
  TestSequenceInput tsi(m_coder->syndromeMgr()->get_n());
  tsi.read_from_file(file);

  int si_count = std::min(tsi.get_side_info_count(), max_si_count);

  std::vector<SIVec> side_infos;

  // encode for each SI
  for (int k = 0; k < si_count; k++) {
    side_infos.push_back(tsi.get_side_info(k));
  }

  std::vector<NumcodeIdxVec> numcodes;
  std::vector<SyndromesVec> syndromes;
  BitSymbolProvider::params bsp_params = {enable_z_quantization, q_values};

  m_coder->encode_symboles(numcodes, syndromes, side_infos, bsp_params,
                           tsi.get_source());

  ICOV_INFO << "Check incremental syndrome";
  // CHECK INCREMENTAL SYNDROME
  int bitplanes_count = syndromes[0].size();
  for (int i = 0; i < bitplanes_count; i++) {

    std::cout << "BITPLANE " << i << std::endl;
    // check per bitplane then per SI
    int sk = 0;
    bool sk_first = true;
    bool sk_val = false;
    do {
      sk_first = true;

      NumcodeIdx max_rate = 0;
      for (int k = 0; k < (int)syndromes.size(); ++k) {

        if (numcodes[k][i] > max_rate)
          max_rate = numcodes[k][i];

        //        for (int l = 0; l < (int)syndromes[k][i].size(); ++l) {
        //          std::cout << syndromes[k][i][l] << " ";
        //        }

        //        std::cout << std::endl;

        //        if ((int)syndromes[k][i].size() > sk) {

        //          if (sk_first) {
        //            sk_val = syndromes[k][i][sk];
        //            sk_first = false;
        //          } else {
        //            icov::helpers::assert::equal(sk_val, syndromes[k][i][sk],
        //                                         "Incremental syndrome check
        //                                         failed");
        //          }
        //        }
      }

      ++sk;
    } while (!sk_first);
  }
  ICOV_INFO << "Incremental syndrome OK";

  write_results(fs::path(file).stem().string(), numcodes);
}

const std::string SymboleMode::dataset_dir_id = "symbole-data-dir";
const std::string SymboleMode::reference_dir_id = "symbole-reference-dir";
const std::string SymboleMode::result_dir_id = "symbole-result-dir";
const std::string SymboleMode::max_si_count_id = "symbole-si-count";
const std::string SymboleMode::enable_q_quantization_id = "symbole-q-quant";
const std::string SymboleMode::enable_z_quantization_id = "symbole-z-quant";
