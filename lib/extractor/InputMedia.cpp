#include "InputMedia.h"

#include "Media.h"
#include "icov_assert.h"
#include "icov_log.h"
#include "icov_path.h"
#include "version.h"
#include "video_layer/ContentParameterSet.h"
#include "video_layer/ips.h"
#include "video_layer/isobmff.h"

#include <boost/filesystem/path.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>

namespace isobmff = icov::bitstream::isobmff;

icov::extractor::InputIcov::InputIcov(const boost::filesystem::path &path)
    : icov::IInputIcov<pstream>(path), m_blockCount(0), m_blockPos(0),
      m_syndromePos(0) {
  path::assert_file_exists(path.string());
  m_file.open(path.string(), std::ios::binary);

  std::vector<uint8_t> moov;
  bool has_ftyp = false;
  isobmff::BoxInfo b;
  while (isobmff::read_box(m_file, b)) {
    if (b.type == "ftyp") {
      std::vector<char> brand(4);
      m_file.read(brand.data(), 4);
      has_ftyp = std::string(brand.data(), 4) == isobmff::BRAND;
    } else if (b.type == "moov") {
      moov.resize(b.payload_size());
      m_file.read(reinterpret_cast<char *>(moov.data()), moov.size());
    }
    m_file.clear();
    m_file.seekg(b.offset + b.size);
  }
  m_file.clear();
  if (!has_ftyp || moov.empty())
    ICOV_THROW_INVALID_ARGUMENT("Not an ICOV file: " + path.string());

  for (const auto &box : isobmff::list_boxes(moov)) {
    // FullBox: skip version and flags
    const uint8_t *p = moov.data() + box.payload_offset() + 4;
    const size_t size = box.payload_size() - 4;
    if (box.type == "icvC") {
      m_ips.assign(p, p + size);
    } else if (box.type == "ihsh") {
      const uint32_t frames = isobmff::get_u32(p);
      const uint32_t blocks = isobmff::get_u32(p + 4);
      if (8 + uint64_t(frames) * blocks * 4 > size)
        ICOV_THROW_RUNTIME_EXCEPTION("Truncated block hashes");
      m_hash.assign(frames, std::vector<uint32_t>(blocks));
      for (uint32_t f = 0; f < frames; ++f)
        for (uint32_t i = 0; i < blocks; ++i)
          m_hash[f][i] = isobmff::get_u32(p + 8 + (f * blocks + i) * 4);
    } else if (box.type == "icbi") {
      const uint32_t frames = isobmff::get_u32(p);
      m_blockCount = isobmff::get_u32(p + 4);
      m_index.resize(size_t(frames) * m_blockCount);
      if (8 + m_index.size() * 16 > size)
        ICOV_THROW_RUNTIME_EXCEPTION("Truncated block index");
      for (size_t i = 0; i < m_index.size(); ++i) {
        const uint8_t *e = p + 8 + i * 16;
        m_index[i] = {isobmff::get_u64(e), isobmff::get_u32(e + 8),
                      isobmff::get_u32(e + 12)};
      }
    }
  }
  if (m_ips.empty() || m_index.empty())
    ICOV_THROW_INVALID_ARGUMENT("Missing parameter set or block index in " +
                                path.string());

  m_contentParameterSet = bitstream::read_ips(m_ips);
  m_frameCount = m_contentParameterSet.content_framecount;
  m_frameID = 0;

  if (m_contentParameterSet.icov_version != ICOV_CORE_VERSION) {
    ICOV_WARNING << "Decoder version " << ICOV_CORE_VERSION
                 << " differs from encoder version "
                 << m_contentParameterSet.icov_version;
  }

  readStatistics();
}

icov::extractor::InputIcov::~InputIcov() {}

void icov::extractor::InputIcov::readStatistics() {
  fs::path stats = getPath();
  stats.replace_extension(".stats.json");
  if (!fs::is_regular_file(stats))
    return;

  namespace pt = boost::property_tree;
  pt::ptree root;
  try {
    pt::read_json(stats.string(), root);
    for (const auto &frame : root.get_child("frames")) {
      m_mse.emplace_back();
      for (const auto &v : frame.second.get_child("mse"))
        m_mse.back().push_back(v.second.get_value<float>());
    }
  } catch (const std::exception &e) {
    ICOV_WARNING << "Ignoring statistics file " << stats.filename() << ": "
                 << e.what();
    m_mse.clear();
  }
}

const icov::bitstream::ContentParameterSet &
icov::extractor::InputIcov::contentParameterSet() const {
  return m_contentParameterSet;
}

icov::bitstream::ContentParameterSet &
icov::extractor::InputIcov::contentParameterSet() {
  return m_contentParameterSet;
}

const std::vector<uint8_t> &
icov::extractor::InputIcov::parameterSetBytes() const {
  return m_ips;
}

void icov::extractor::InputIcov::setFrame(frame_t frame) {
  if (size_t(frame) * m_blockCount >= m_index.size())
    ICOV_THROW_OUT_OF_RANGE("Frame " + std::to_string(frame) +
                            " is not in the stream");
  IInputIcov<pstream>::setFrame(frame);
}

void icov::extractor::InputIcov::selectBlock(
    const omnidirectional_layer::Block::id_t &id) {
  m_blockPos = getBlockIndexInStream(id);
  const auto &e = m_index.at(size_t(m_frameID) * m_blockCount + m_blockPos);

  m_sbu.resize(size_t(e.params_size) + e.syndrome_size);
  m_file.clear();
  m_file.seekg(e.offset);
  if (!m_file.read(reinterpret_cast<char *>(m_sbu.data()), m_sbu.size()))
    ICOV_THROW_RUNTIME_EXCEPTION("Truncated block " + std::to_string(id));

  m_params.reset(m_sbu.data(), e.params_size);
  m_syndromePos = e.params_size;
}

void icov::extractor::InputIcov::jumpPrediction(int bitplaneCount) {
  namespace bs = bitstream::block_syntax;
  if (bs::read_mode(m_params) == 0) // prediction not available
    return;
  if (bs::read_pred_header(m_params) == 0) // not coded
    return;
  bs::read_z(m_params);
  bs::read_z(m_params);
  for (int i = 0; i < bitplaneCount; ++i)
    bs::read_rate(m_params);
}

void icov::extractor::InputIcov::readSyndrome(icov::byte *output,
                                              uint64_t read_size,
                                              uint64_t stored_size) {
  if (m_syndromePos + stored_size > m_sbu.size())
    ICOV_THROW_OUT_OF_RANGE("Syndrome read past the end of the block");
  std::memcpy(output, m_sbu.data() + m_syndromePos, read_size);
  m_syndromePos += stored_size;
}

void icov::extractor::InputIcov::offsetSyndrome(uint offset) {
  m_syndromePos += offset;
}

float icov::extractor::InputIcov::blockMse() const {
  if (size_t(m_frameID) < m_mse.size() &&
      m_blockPos < m_mse[m_frameID].size())
    return m_mse[m_frameID][m_blockPos];
  return std::numeric_limits<float>::quiet_NaN();
}

void icov::extractor::InputIcov::writeInfos(std::ofstream &output, int key,
                                            void *param) {}

icov::omnidirectional_layer::Block::id_t
icov::extractor::InputIcov::getBlockIndexInStream(
    const omnidirectional_layer::Block::id_t &id) const {
  if (m_contentParameterSet.icu_count == 0)
    return id - 1;

#if ICOV_ASSERTION_ENABLED
  ICOV_ASSERT_MSG(m_contentParameterSet.encoded_blocks.count(id),
                  "Block not encoded: " + std::to_string(id));
#endif
  return m_contentParameterSet.encoded_blocks.at(id);
}

void icov::extractor::InputIcov::resetStreams() {
  m_params.reset(nullptr, 0);
  m_sbu.clear();
  m_syndromePos = 0;
}
