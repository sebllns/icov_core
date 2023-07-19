#include "BinaryOutputIcov.h"

#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include "video_layer/encoded_stream.h"
#include "video_layer/icov_stream.h"
#include <boost/serialization/array_wrapper.hpp>
#include <ios>
#include <limits>
#include <memory>
#include <string>
namespace icov {
namespace encoder {

BinaryOutputIcov::BinaryOutputIcov(const boost::filesystem::path &path)
    : OutputIcovBase(path, std::ios_base::binary) {}

void BinaryOutputIcov::writeQaryParams(
    const ldpc_codec_layer::QaryDistribution &bsp) {
  if (!bsp.codingflag()) {
    // std::cout << "SI IDENTICAL" << std::endl;
    std::cout << "Q 0" << std::endl;
    icov::io::write_value<bitstream::byte>(
        getStream(bitstream::eIcovStream::ENCODE_PARAMS), 0, true);
    m_total_write_byte[eWriteCounter::BLOCK_RATE] += sizeof(bitstream::byte);
  } else {
    icov::bitstream::writeQaryParams(m_qaryParams, bsp.getRawQ0(),
                                     bsp.getRawZmin(), bsp.getRawZmax());
    icov::io::write_array(getStream(bitstream::eIcovStream::ENCODE_PARAMS),
                          m_qaryParams, QARY_PARAMS_SIZE, true);
    m_total_write_byte[eWriteCounter::BLOCK_RATE] +=
        QARY_PARAMS_SIZE * sizeof(bitstream::byte);
  }
}

void BinaryOutputIcov::writeContentParameterSet() {
  boost::archive::binary_oarchive o_archive(
      *m_fileParts[bitstream::eIcovStream::MEDIA_HEADER],
      boost::archive::no_header);
  o_archive &m_contentParameterSet;
  flush(bitstream::eIcovStream::MEDIA_HEADER);
}

void BinaryOutputIcov::writeSyndrome(const Syndrome &s) {
  const int byte_size = bitstream::bitToByteCount(s.size());
  bitstream::byte darr[byte_size];

  std::fill(darr, darr + byte_size, 0);

  bitstream::write_bits(s, darr, 0);

  int lost = bitstream::byteToBitCount(byte_size) - s.size();

  m_total_write_byte[eWriteCounter::BLOCK_SYNDROME] += byte_size;

  //  std::cout << "WRITE BYTES " << byte_size << std::endl;
  //  std::cout << "WRITE BITS " << s.size() << std::endl;
  //  std::cout << "SHIFT BITS " << last_bit_shift << std::endl;
  //  std::cout << "LOST BITS " << lost << std::endl;

  //  std::cout << "TOTAL WRITE BITS " << total_write_bits << std::endl;
  //  std::cout << "TOTAL SHIFT BITS " << total_shift << std::endl;
  //  std::cout << "TOTAL LOST BITS " << total_lost << std::endl;

  //  boost::archive::binary_oarchive
  //  o_archive(*m_fileParts[eIcovStream::SYNDROME],
  //                                            boost::archive::no_header);
  //  o_archive &boost::serialization::make_array(darr, byte_size);

  //  m_fileParts[eIcovStream::SYNDROME]->write((char *)darr, sizeof(darr));
  //  m_fileParts[eIcovStream::SYNDROME]->flush();

  icov::io::write_array(*m_fileParts[bitstream::eIcovStream::SYNDROME], darr,
                        byte_size, true);
}

void BinaryOutputIcov::writeFrameSize() {
  icov::io::write_value<uint32_t>(
      *m_fileParts[bitstream::eIcovStream::FRAMES_SIZE],
      m_total_write_byte[eWriteCounter::FRAME_RATE], false);
  icov::io::write_value<uint32_t>(
      *m_fileParts[bitstream::eIcovStream::FRAMES_SIZE],
      m_total_write_byte[eWriteCounter::FRAME_SYNDROME], true);

  m_total_write_byte[eWriteCounter::FRAME_RATE] = 0;
  m_total_write_byte[eWriteCounter::FRAME_SYNDROME] = 0;
}

void BinaryOutputIcov::writeBlockSize() {

  std::cout << "WRITE SIZE " << m_total_write_byte[eWriteCounter::BLOCK_RATE]
            << " " << m_total_write_byte[eWriteCounter::BLOCK_SYNDROME]
            << std::endl;

  icov::io::write_value<uint32_t>(
      *m_fileParts[bitstream::eIcovStream::BLOCKS_SIZE],
      m_total_write_byte[eWriteCounter::BLOCK_RATE], true);
  icov::io::write_value<uint32_t>(
      *m_fileParts[bitstream::eIcovStream::BLOCKS_SIZE],
      m_total_write_byte[eWriteCounter::BLOCK_SYNDROME], true);

  m_total_write_byte[eWriteCounter::FRAME_RATE] +=
      m_total_write_byte[eWriteCounter::BLOCK_RATE];
  m_total_write_byte[eWriteCounter::FRAME_SYNDROME] +=
      m_total_write_byte[eWriteCounter::BLOCK_SYNDROME];

  m_total_write_byte[eWriteCounter::BLOCK_RATE] = 0;
  m_total_write_byte[eWriteCounter::BLOCK_SYNDROME] = 0;
}

void BinaryOutputIcov::writeBitplaneCount(const uint &v) {
  writeSIRate(v, 0, 0);
}

// void BinaryOutputIcov::writeSIRates(const NumcodeIdxVec &v) {
//  rates_count += v.size() * sizeof(rate_t);
//  // CHECKED RATES
//  std::cout << "WRITE RATES";
//  for (auto r : v) {
//    writeSIRate(r);
//  }
//  std::cout << std::endl;
//}

void BinaryOutputIcov::writeSIRate(const NumcodeIdx &v, const Rate &r,
                                   const uint &n) {
#ifdef ICOV_ASSERTION_ENABLED
  icov::assert::check(r < std::numeric_limits<rate_t>::max(),
                      "Value " + std::to_string(r) +
                          " overflows the max possible value " +
                          std::to_string(std::numeric_limits<rate_t>::max()));
#endif

  icov::io::write_value<rate_t>(
      *m_fileParts[bitstream::eIcovStream::ENCODE_PARAMS],
      static_cast<rate_t>(v), true);

  std::cout << " " << v;
  m_total_write_byte[eWriteCounter::BLOCK_RATE] += sizeof(rate_t);
}

void BinaryOutputIcov::writeSIMode(const uint &v) {
  icov::io::write_value<bitstream::byte>(
      *m_fileParts[bitstream::eIcovStream::ENCODE_PARAMS],
      static_cast<rate_t>(v), true);
  m_total_write_byte[eWriteCounter::BLOCK_RATE] += sizeof(bitstream::byte);
}

} // namespace encoder
} // namespace icov

void icov::encoder::BinaryOutputIcov::startWriteBlock(
    const icov::omnidirectional_layer::Block &b) {}

void icov::encoder::BinaryOutputIcov::startWritePrediction(
    icov::omnidirectional_layer::IBlockPredictionManager::prediction_t pred) {}

void icov::encoder::BinaryOutputIcov::endWritePrediction(
    omnidirectional_layer::IBlockPredictionManager::prediction_t pred) {}

void icov::encoder::BinaryOutputIcov::endWriteBlock(
    const omnidirectional_layer::Block &b) {}

void icov::encoder::BinaryOutputIcov::writeStoredSyndromeSize(
    const NumcodeIdx &v, const Rate &r, const uint &m) {
  writeSIRate(v, 0, 0);
}
