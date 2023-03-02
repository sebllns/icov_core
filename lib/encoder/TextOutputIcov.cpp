#include "TextOutputIcov.h"
#include "Block.h"
#include "encoder/OutputIcovBase.h"
#include "helpers.h"
#include "icov_stream.h"
#include "typedef.h"
#include "video_layer/encoded_stream.h"
#include <algorithm>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/array_wrapper.hpp>
#include <limits>
#include <memory>
#include <string>
namespace icov {
namespace encoder {

TextOutputIcov::TextOutputIcov(const boost::filesystem::path &path)
    : OutputIcovBase(path, 0) {}

void TextOutputIcov::writeQaryParams(const BitSymbolProvider::Pz &bsp) {
  if (bsp.lx == 0) {
    // std::cout << "SI IDENTICAL" << std::endl;
    getStream(eIcovStream::ENCODE_PARAMS) << "Q 0 ";
    m_total_write_byte[eWriteCounter::BLOCK_RATE] += sizeof(icov::byte);
  } else {
    getStream(eIcovStream::ENCODE_PARAMS)
        << "Q " << bsp.q_idx << " Zmin " << bsp.zmin << " Zmax " << bsp.zmax
        << " ";

    m_total_write_byte[eWriteCounter::BLOCK_RATE] +=
        QARY_PARAMS_SIZE * sizeof(icov::byte);
  }
}

void TextOutputIcov::writeContentParameterSet() {
  boost::archive::text_oarchive o_archive(
      *m_fileParts[eIcovStream::MEDIA_HEADER], boost::archive::no_header);
  o_archive &m_contentParameterSet;
}

void TextOutputIcov::writeSyndrome(const Syndrome &s) {
  const int byte_size = bitToByteCount(s.size());

  m_total_write_byte[eWriteCounter::BLOCK_SYNDROME] += byte_size;

  getStream(eIcovStream::SYNDROME) << s.to_string(" ");
}

void TextOutputIcov::writeFrameSize() {
  //  helpers::io::write_value<uint32_t>(*m_fileParts[eIcovStream::FRAMES_SIZE],
  //                                    m_total_write_byte[eWriteCounter::FRAME_RATE],
  //                                    false);
  //  helpers::io::write_value<uint32_t>(*m_fileParts[eIcovStream::FRAMES_SIZE],
  //                                     m_total_write_byte[eWriteCounter::FRAME_SYNDROME],
  //                                     true);
  m_total_write_byte[eWriteCounter::FRAME_RATE] = 0;
  m_total_write_byte[eWriteCounter::FRAME_SYNDROME] = 0;
}

void TextOutputIcov::writeBlockSize() {

  std::cout << "WRITE SIZE " << m_total_write_byte[eWriteCounter::BLOCK_RATE]
            << " " << m_total_write_byte[eWriteCounter::BLOCK_SYNDROME]
            << std::endl;

  //  helpers::io::write_value<uint32_t>(*m_fileParts[eIcovStream::BLOCKS_SIZE],
  //                                      m_total_write_byte[eWriteCounter::BLOCK_RATE],
  //                                      true);
  //  helpers::io::write_value<uint32_t>(*m_fileParts[eIcovStream::BLOCKS_SIZE],
  //                                     m_total_write_byte[eWriteCounter::BLOCK_SYNDROME],
  //                                     true);

  m_total_write_byte[eWriteCounter::FRAME_RATE] +=
      m_total_write_byte[eWriteCounter::BLOCK_RATE];
  m_total_write_byte[eWriteCounter::FRAME_SYNDROME] +=
      m_total_write_byte[eWriteCounter::BLOCK_SYNDROME];

  m_total_write_byte[eWriteCounter::BLOCK_RATE] = 0;
  m_total_write_byte[eWriteCounter::BLOCK_SYNDROME] = 0;
}

void TextOutputIcov::writeSIRate(const NumcodeIdx &r) {
  getStream(eIcovStream::ENCODE_PARAMS) << r << " ";
  m_total_write_byte[eWriteCounter::BLOCK_RATE] += sizeof(rate_t);
}

void TextOutputIcov::writeSIMode(const uint &v) {
  getStream(eIcovStream::ENCODE_PARAMS) << "MODE " << v << " ";
  m_total_write_byte[eWriteCounter::BLOCK_RATE] += sizeof(icov::byte);
}

} // namespace encoder
} // namespace icov

void icov::encoder::TextOutputIcov::startWriteBlock(const Block &b) {
  getStream(eIcovStream::ENCODE_PARAMS)
      << std::endl
      << "BLOCK " << b.getIdSelf() << std::endl;
}

void icov::encoder::TextOutputIcov::startWritePrediction(uint16_t pred) {
  getStream(eIcovStream::ENCODE_PARAMS) << " PRED " << pred << " ";
}

void icov::encoder::TextOutputIcov::endWritePrediction(uint16_t pred) {}

void icov::encoder::TextOutputIcov::endWriteBlock(const Block &b) {}
