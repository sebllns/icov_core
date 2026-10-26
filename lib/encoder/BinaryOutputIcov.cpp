#include "BinaryOutputIcov.h"

#include "EncoderStatistics.h"
#include "icov_exception.h"
#include "icov_log.h"
#include "video_layer/encoded_stream.h"
#include "video_layer/icov_stream.h"
#include "video_layer/ips.h"
#include "video_layer/isobmff.h"

#include <boost/filesystem/operations.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <limits>
#include <string>

namespace icov {
namespace encoder {

namespace isobmff = bitstream::isobmff;

BinaryOutputIcov::BinaryOutputIcov(const boost::filesystem::path &path)
    : OutputIcovBase(path), m_payloadSize(0), m_closed(false) {
  m_payloadPath = path;
  m_payloadPath += ".payload.tmp";
  m_payload.open(m_payloadPath.string(), std::ios::binary | std::ios::trunc);
  if (!m_payload)
    ICOV_THROW_RUNTIME_EXCEPTION("Cannot create " + m_payloadPath.string());
}

BinaryOutputIcov::~BinaryOutputIcov() {
  if (m_payload.is_open())
    m_payload.close();
  boost::system::error_code ec;
  fs::remove(m_payloadPath, ec);
}

void BinaryOutputIcov::writeByte(unsigned v) {
  if (v > std::numeric_limits<rate_t>::max())
    ICOV_THROW_OUT_OF_RANGE("Value " + std::to_string(v) +
                            " overflows the max possible value " +
                            std::to_string(std::numeric_limits<rate_t>::max()));
  m_params.u(8 * sizeof(rate_t), v);
}

void BinaryOutputIcov::writeQaryParams(
    const ldpc_codec_layer::QaryDistribution &bsp) {
  if (!bsp.codingflag()) {
    writeByte(0);
    return;
  }
  bitstream::QaryParams q;
  bitstream::writeQaryParams(q, bsp.getRawQ0(), bsp.getRawZmin(),
                             bsp.getRawZmax());
  for (int i = 0; i < QARY_PARAMS_SIZE; ++i)
    writeByte(q[i]);
}

void BinaryOutputIcov::writeSyndrome(const Syndrome &s) {
  // each syndrome stays byte-aligned so that the extractor can slice it
  const size_t first = m_syndromes.size();
  m_syndromes.resize(first + bitstream::bitToByteCount(s.size()), 0);
  bitstream::write_bits(s, m_syndromes.data() + first, 0);
}

void BinaryOutputIcov::writeFrameSize() {
  // block order inside a frame follows the parameter set; nothing to index
}

void BinaryOutputIcov::writeBlockSize() {
  m_params.byte_alignment();
  const auto &params = m_params.data();

  m_index.push_back({m_payloadSize, uint32_t(params.size()),
                     uint32_t(m_syndromes.size())});
  m_payload.write(reinterpret_cast<const char *>(params.data()), params.size());
  m_payload.write(reinterpret_cast<const char *>(m_syndromes.data()),
                  m_syndromes.size());
  m_payloadSize += params.size() + m_syndromes.size();

  m_params.clear();
  m_syndromes.clear();
}

void BinaryOutputIcov::writeBitplaneCount(const uint &v) { writeByte(v); }

void BinaryOutputIcov::writeSIRate(const NumcodeIdx &v, const Rate &,
                                   const uint &) {
  writeByte(v);
}

void BinaryOutputIcov::writeSIMode(const uint &v) { writeByte(v); }

void BinaryOutputIcov::startWriteBlock(
    const icov::omnidirectional_layer::Block &) {}

void BinaryOutputIcov::startWritePrediction(
    icov::omnidirectional_layer::IBlockPredictionManager::prediction_t) {}

void BinaryOutputIcov::endWritePrediction(
    omnidirectional_layer::IBlockPredictionManager::prediction_t) {}

void BinaryOutputIcov::endWriteBlock(
    const icov::omnidirectional_layer::Block &) {}

void BinaryOutputIcov::writeStoredSyndromeSize(const NumcodeIdx &v,
                                               const Rate &r, const uint &m) {
  writeSIRate(v, r, m);
}

void BinaryOutputIcov::writeMse(const float &mse) {
  // the encoder writes the MSE of all blocks of a frame before its blocks
  if (m_mse.empty() || m_mse.size() * m_contentParameterSet.encoded_blocks.size() <=
                           m_index.size())
    m_mse.emplace_back();
  m_mse.back().push_back(mse);
}

void BinaryOutputIcov::endIcov() {}

void BinaryOutputIcov::close() {
  if (m_closed)
    return;
  m_closed = true;
  m_payload.close();

  const auto &cps = m_contentParameterSet;
  const size_t blocks = cps.encoded_blocks.size();
  if (blocks == 0 || m_index.size() % blocks != 0)
    ICOV_THROW_LOGICAL_EXCEPTION("Block index does not match the coded blocks");

  std::vector<uint8_t> index;
  isobmff::put_u32(index, uint32_t(m_index.size() / blocks));
  isobmff::put_u32(index, uint32_t(blocks));
  // offsets are patched below once the payload start is known
  std::vector<uint8_t> moov_payload;
  const auto icvC = isobmff::full_box("icvC", 0, bitstream::write_ips(cps));
  moov_payload.insert(moov_payload.end(), icvC.begin(), icvC.end());

  std::vector<uint8_t> ftyp_payload;
  isobmff::put_type(ftyp_payload, isobmff::BRAND);
  isobmff::put_u32(ftyp_payload, 0);
  isobmff::put_type(ftyp_payload, isobmff::BRAND);
  const auto ftyp = isobmff::box("ftyp", ftyp_payload);

  const uint64_t icbi_size = 8 + 4 + index.size() + m_index.size() * 16;
  const uint64_t moov_size = 8 + moov_payload.size() + icbi_size;
  const auto mdat_header = isobmff::box_header("mdat", m_payloadSize);
  const uint64_t payload_start = ftyp.size() + moov_size + mdat_header.size();

  for (const auto &e : m_index) {
    isobmff::put_u64(index, payload_start + e.offset);
    isobmff::put_u32(index, e.params_size);
    isobmff::put_u32(index, e.syndrome_size);
  }
  const auto icbi = isobmff::full_box("icbi", 0, index);
  moov_payload.insert(moov_payload.end(), icbi.begin(), icbi.end());
  const auto moov = isobmff::box("moov", moov_payload);

  m_outputStream.reset();
  std::ofstream out(getPath().string(), std::ios::binary | std::ios::trunc);
  out.write(reinterpret_cast<const char *>(ftyp.data()), ftyp.size());
  out.write(reinterpret_cast<const char *>(moov.data()), moov.size());
  out.write(reinterpret_cast<const char *>(mdat_header.data()),
            mdat_header.size());
  std::ifstream payload(m_payloadPath.string(), std::ios::binary);
  out << payload.rdbuf();
  if (!out)
    ICOV_THROW_RUNTIME_EXCEPTION("Cannot write " + getPath().string());
  payload.close();
  fs::remove(m_payloadPath);

  writeStats();
  ICOV_INFO << "Wrote " << getPath().filename() << " ("
            << ftyp.size() + moov.size() + mdat_header.size() + m_payloadSize
            << " bytes) and " << statsPath().filename();
}

void BinaryOutputIcov::writeStats() const {
  namespace pt = boost::property_tree;
  pt::ptree root;
  for (const auto &kv : m_info)
    root.put("encoder." + kv.first, kv.second);
  if (m_stats)
    root.add_child("statistics", m_stats->toTree());

  pt::ptree frames;
  for (const auto &f : m_mse) {
    pt::ptree mse;
    for (float v : f) {
      pt::ptree item;
      item.put_value(v);
      mse.push_back({"", item});
    }
    pt::ptree frame;
    frame.add_child("mse", mse);
    frames.push_back({"", frame});
  }
  root.add_child("frames", frames);
  pt::write_json(statsPath().string(), root);
}

} // namespace encoder
} // namespace icov
