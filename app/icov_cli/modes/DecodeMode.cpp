#include "DecodeMode.h"
#include "app_common/decoder/SinglePassDecoder.h"
#include "app_common/program/icov_program_options.h"

#include <boost/format.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <vector>

DecodeMode::DecodeMode(const path::path_t &icov_file, const po::variables_map &args) : m_decoderCtrl(icov_file, args)
{
    ICOV_DEBUG << "Parse program arguments";

    m_decoderCtrl.initAnalyzer();
    m_requests = m_decoderCtrl.readRequests();

    m_verifyHash = options::parseVerifyHash(args);
    if (m_verifyHash) {
      const auto &hashes = m_decoderCtrl.getDecoder()->icovFile()->blockHashes();
      if (hashes.empty())
        ICOV_THROW_INVALID_ARGUMENT("--verify-hash: the file has no block hashes");
      m_decoderCtrl.getDecoder()->blockDecoder()->setExpectedHashes(hashes);
    }
}

void DecodeMode::run() {
  ICOV_DEBUG << "Start decoding ICUs";
  decode();
}

void DecodeMode::decode() {
  m_decoderCtrl.decodePositions(m_requests);

  if (m_verifyHash) {
    const auto &dec = *m_decoderCtrl.getDecoder()->blockDecoder();
    ICOV_INFO << "Hash check: " << dec.checkedBlocks() << " blocks, "
              << dec.hashMismatches() << " mismatches";
    if (dec.checkedBlocks() == 0 || dec.hashMismatches() > 0)
      ICOV_THROW_RUNTIME_EXCEPTION("Decoded blocks differ from the encoder");
  }
}
