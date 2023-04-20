#include "BinaryMode.h"

#include "icov_program/icov_program_options.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "tools/DebugTools.h"
#include <chrono>

void BinaryMode::AddProgramOptions(
    boost::program_options::options_description &desc) {
  desc.add_options()("binary-iterations", po::value<int>()->default_value(100),
                     "set iteration count for test loop");
}

BinaryMode::BinaryMode(const po::variables_map &args)
    : m_coder(icov::options::create_coder(args)),
      m_src_bits(m_coder->syndromeMgr()->get_n()),
      m_dec_bits(m_coder->syndromeMgr()->get_n()),
      m_stored_syndrome(m_coder->syndromeMgr()->get_n()), m_numcodeIdx(0),
      m_error_count(0) {
  m_nite = args["binary-iterations"].as<int>();
}

void BinaryMode::run() {
  ICOV_DEBUG << "encode\n";
  auto llrProvider(m_coder->llrProvider());
  auto syndromeMgr(m_coder->syndromeMgr());
  auto matrixProvider(m_coder->matrixProvider());

  int n = syndromeMgr->get_n();

  fill_pcrossover();

  // loop
  for (int ite = 0; ite < m_nite; ite++) {
    fill_src();
    syndromeMgr->init_accumulation(m_src_bits);

    m_numcodeIdx = 0;
    for (int i = 0; i < (int)m_pcrossover.size(); i++) {
      float progress = 100.0 * (ite * m_pcrossover.size() + i) /
                       (m_pcrossover.size() * m_nite);
      std::cout << "    " << progress << "%                \r" << std::flush;

      ICOV_TRACE << (ite * m_pcrossover.size() + i + 1) << "/"
                 << (m_pcrossover.size() * m_nite);

      ICOV_TRACE << "Crossover probability " << m_pcrossover[i] << std::endl;
      // init llr
      llrProvider->initLLRUniform(m_pcrossover[i], m_src_bits);
      //      llr = llrProvider->llr();
      syndromeMgr->reset_doping();

      auto start = std::chrono::steady_clock::now();
      ICOV_TRACE << "encode";
      int err_count =
          m_coder->encode(m_numcodeIdx, m_stored_syndrome, m_src_bits);
      auto end = std::chrono::steady_clock::now();

      std::chrono::duration<double> elapsed_seconds = end - start;
      rate[i] += m_coder->get_r(m_numcodeIdx) / m_nite;
      time[i] +=
          elapsed_seconds
              .count(); /// p.n_ite_crossover;// + (i > 0 ? b.time[i-1] : 0.0);
      numerrors[i] += (1.0 * err_count) / (m_nite * n);

      if (err_count != 0) {
        // raise error
        ICOV_ERROR << "No convergence. Pcrossover=" << m_pcrossover[i];
      } else {
        start = std::chrono::steady_clock::now();
        ICOV_TRACE << "decode";
        m_coder->decode(m_dec_bits, m_numcodeIdx, m_stored_syndrome);
        end = std::chrono::steady_clock::now();
        elapsed_seconds = end - start;
        decoding_time[i] += elapsed_seconds.count() / m_nite;
        if (!m_coder->testConvergence(m_dec_bits, m_src_bits)) {
          ICOV_ERROR << "Could not decode!" << std::endl;
        }
      }
    }
  }

  write_results();
}

void BinaryMode::fill_src() {
  std::generate(m_src_bits.begin(), m_src_bits.end(), []() {
    return (icov::BitVal)(((((double)rand() / (double)RAND_MAX) < 0.5)) ? 1.0
                                                                        : 0.0);
  });
}

void BinaryMode::fill_pcrossover() {
  ICOV_DEBUG << CONSOLE_TODO << "TODO: read crossover probability from file !";
  m_pcrossover = {0.005607170053021, 0.012986862055522, 0.021539631929892,
                  0.031124460304790, 0.041692690273656, 0.053239040776806,
                  0.065786706354577, 0.079382600480655, 0.094097243346146,
                  0.110027864438359, 0.127304805975877, 0.146102403411897,
                  0.166657010396903, 0.189297705370615, 0.214501744859832,
                  0.243003853808948, 0.276040891839788, 0.316019346323608,
                  0.369127748984521, 0.500000000000000};

  rate = std::vector<double>(m_pcrossover.size(), 0);
  numerrors = std::vector<double>(m_pcrossover.size(), 0);
  time = std::vector<double>(m_pcrossover.size(), 0);
  decoding_time = std::vector<double>(m_pcrossover.size(), 0);
}

void BinaryMode::write_results() {
  ICOV_INFO << "End of test. Write results.";

  for (size_t i = 1; i < m_pcrossover.size(); i++) {
    time[i] += time[i - 1];
  }

  for (size_t i = 0; i < m_pcrossover.size(); i++) {
    time[i] /= m_nite;
  }

  DebugTools::write_vector("pcrossover", m_pcrossover, '\n');
  DebugTools::write_vector("rate", rate, '\n');
  DebugTools::write_vector("numerrors", numerrors, '\n');
  DebugTools::write_vector("encode_time", time, '\n');
  DebugTools::write_vector("decode_time", decoding_time, '\n');
}
