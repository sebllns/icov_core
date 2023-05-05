#include "TextOutputIcov.h"
#include "encoder/OutputIcovBase.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include "video_layer/encoded_stream.h"
#include "video_layer/icov_stream.h"

#include <boost/archive/text_oarchive.hpp>
#include <boost/json/array.hpp>
#include <boost/json/detail/object.hpp>
#include <boost/json/object.hpp>
#include <boost/json/src.hpp>
#include <boost/serialization/array_wrapper.hpp>

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
namespace icov {
namespace encoder {

TextOutputIcov::TextOutputIcov(const boost::filesystem::path &path)
    : OutputIcovBase(path, 0) {}

void TextOutputIcov::writeQaryParams(
    const ldpc_codec_layer::BitSymbolProvider::PzParams &bsp) {
  if (bsp.getZAmplitude() == 0) {
    // std::cout << "SI IDENTICAL" << std::endl;
    getStream(bitstream::eIcovStream::ENCODE_PARAMS) << "Q 0 ";
    m_total_write_byte[eWriteCounter::BLOCK_RATE] += sizeof(bitstream::byte);
  } else {
    getStream(bitstream::eIcovStream::ENCODE_PARAMS)
        << "Q " << bsp.getQ0() << " Zmin " << bsp.getZmin() << " Zmax "
        << bsp.getZmax() << " ";

    m_total_write_byte[eWriteCounter::BLOCK_RATE] +=
        QARY_PARAMS_SIZE * sizeof(bitstream::byte);
  }
}

void TextOutputIcov::writeContentParameterSet() {
  boost::archive::text_oarchive o_archive(
      *m_fileParts[bitstream::eIcovStream::MEDIA_HEADER],
      boost::archive::no_header);
  o_archive &m_contentParameterSet;
}

void TextOutputIcov::writeSyndrome(const Syndrome &s) {
  const int byte_size = bitstream::bitToByteCount(s.size());

  m_total_write_byte[eWriteCounter::BLOCK_SYNDROME] += byte_size;

  getStream(bitstream::eIcovStream::SYNDROME) << s.to_string(" ");
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

void TextOutputIcov::writeBitplaneCount(const uint &v) {
  getChannelNodeJSON()["bitplane_count"] = v;
}

void TextOutputIcov::writeStoredSyndromeSize(const uint &v) {}

void TextOutputIcov::writeSIRate(const uint &r) {
  getStream(bitstream::eIcovStream::ENCODE_PARAMS) << r << " ";
  m_total_write_byte[eWriteCounter::BLOCK_RATE] += sizeof(rate_t);
}

void TextOutputIcov::writeSIMode(const uint &v) {
  getStream(bitstream::eIcovStream::ENCODE_PARAMS) << "MODE " << v << " ";
  m_total_write_byte[eWriteCounter::BLOCK_RATE] += sizeof(bitstream::byte);
}

} // namespace encoder
} // namespace icov

void icov::encoder::TextOutputIcov::startWriteBlock(
    const omnidirectional_layer::Block &b) {
  m_blockNodeJSON.clear();
  m_blockNodeJSON["block_id"] = b.getIdSelf();
  m_blockNodeJSON["channels"] = boost::json::object();
  m_blockNodeJSON["is_access_block"] = false;
}

void icov::encoder::TextOutputIcov::startWritePrediction(
    icov::omnidirectional_layer::IBlockPredictionManager::prediction_t pred) {
  getStream(bitstream::eIcovStream::ENCODE_PARAMS) << " PRED " << pred << " ";
  m_currentPrednodeJSON.clear();

  auto pred_type = omnidirectional_layer::BlockPrediction::toPredType(pred);
  getChannelNodeJSON()[omnidirectional_layer::BlockPrediction::toString(
      pred_type)] = m_currentPrednodeJSON;

  if (pred_type == omnidirectional_layer::BlockPrediction::ePredType::NONE) {
    m_blockNodeJSON["is_access_block"] = true;
  }
}

void icov::encoder::TextOutputIcov::endWritePrediction(
    omnidirectional_layer::IBlockPredictionManager::prediction_t pred) {}

void icov::encoder::TextOutputIcov::endWriteBlock(
    const omnidirectional_layer::Block &b) {
  getStream(bitstream::eIcovStream::ENCODE_PARAMS) << m_blockNodeJSON;
}

boost::json::object &icov::encoder::TextOutputIcov::getChannelNodeJSON() {

  if (!m_blockNodeJSON["channels"]
           .as_object()[std::to_string(channel())]
           .is_object()) {
    m_blockNodeJSON["channels"].as_object()[std::to_string(channel())] =
        boost::json::object();
  }

  return m_blockNodeJSON["channels"]
      .as_object()[std::to_string(channel())]
      .as_object();
}
