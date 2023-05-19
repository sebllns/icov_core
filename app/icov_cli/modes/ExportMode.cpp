#include "ExportMode.h"
#include "icov_program/icov_program_options.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "ldpc_codec_layer/io/Alist.h"

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

const std::string ExportMode::rate_min_id = "export-min-rate";
const std::string ExportMode::rate_max_id = "export-max-rate";
const std::string ExportMode::rate_doping_id = "export-doping-rate";
const std::string ExportMode::export_matrix_id = "export-matrix";
const std::string ExportMode::export_numcode_id = "export-numcode";

void ExportMode::AddProgramOptions(
    boost::program_options::options_description &desc) {
  desc.add_options()(export_matrix_id.c_str(),
                     po::value<bool>()->default_value(false),
                     "Enable matrices export")(
      export_numcode_id.c_str(), po::value<int>()->default_value(2),
      "Enable numcode export / 0 = none, 1 = all, 2 = auto")(
      rate_min_id.c_str(), po::value<float>(), "rate min")(
      rate_max_id.c_str(), po::value<float>()->default_value(1.5),
      "rate max")(rate_doping_id.c_str(),
                  po::value<float>()->default_value(0.5), "rate doping");
}

ExportMode::ExportMode(const boost::program_options::variables_map &args,
                       int code_length, const std::string &data_path)
    : n(code_length), m_options_map(args), datapath(data_path) {
  export_matrix_enabled = args[export_matrix_id].as<bool>();
  export_numcode_mode = args[export_numcode_id].as<int>();
  min_rate = args[rate_min_id].as<float>();
  max_rate = args[rate_max_id].as<float>();
  doping_rate = args[rate_doping_id].as<float>();
  numcodes_file = args[OPT_NUMCODES].as<std::string>();
}

void ExportMode::run() {
  export_numcode();
  if (export_matrix_enabled) {
    m_coder = icov::options::create_coder(m_options_map);
    export_matrices();
  }
}

void ExportMode::export_matrices() {
  fs::path alist_dir = icov::path::get_data_folder(datapath, n, true);
  alist_dir /= "alist";
  icov::path::try_create_directories(alist_dir.string());

  for (int i = 0; i < m_coder->matrixProvider()->getMatrixCount(); i++) {
    auto m = m_coder->matrixProvider()->getMatrix(i);
    std::string name = "H_" + std::to_string(m.get_n_cols()) + ".alist";
    icov::io::writeAlist((alist_dir / name).string(), m);
  }
}

void ExportMode::export_numcode() {
  std::vector<int> numcodes;
  int n_doping = doping_rate * n;
  int n_max = max_rate * n;
  int n_min = min_rate * n;

  switch (export_numcode_mode) {
  case 0:
    return;
  case 1: {
    numcodes.resize((size_t)((max_rate - min_rate) * n));
    std::iota(numcodes.begin(), numcodes.end(), n_min);
  } break;
  case 2: {
    int step = std::ceil(std::log2f(n) / 8) * 4;

    int m = n_min;
    int m1 = n_doping;

    while (m < m1) {
      for (int i = m; i < m1; i += step) {
        numcodes.push_back(i);
      }

      m = m1;

      if (m1 > n_doping) {
        m1 = n_max;
        step = std::ceil(std::log2f(n) / 8) * 8;
      } else {
        m1 = n;
      }
    }
  }

  break;

  default: {
    BOOST_THROW_EXCEPTION(std::invalid_argument(
        "Unknown numcode export mode: " + std::to_string(export_numcode_mode)));
  }
  }

  numcodes.push_back((int)(doping_rate * n)); // index for the start of doping
  fs::path path = icov::path::get_data_folder(datapath, n, true);
  path /= numcodes_file;
  std::cout << "Writing numcodes in " << path << std::endl;
  icov::io::write_vector(path.string(), numcodes, '\n');
}
