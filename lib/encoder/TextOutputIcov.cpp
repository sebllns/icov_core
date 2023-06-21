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
    const ldpc_codec_layer::QaryDistribution &bsp) {
  if (!bsp.codingflag()) {
    m_currentPrednodeJSON["no_coding_flag"] = true;
  } else {
    m_currentPrednodeJSON["no_coding_flag"] = false;
    boost::json::object pzParamsJSON;
    pzParamsJSON["q0"] = bsp.getQ0();
    pzParamsJSON["zmin"] = bsp.getZmin();
    pzParamsJSON["zmax"] = bsp.getZmax();
    m_currentPrednodeJSON["pz_params"] = pzParamsJSON;
    m_currentPrednodeJSON["syndrome_rate"] = boost::json::array();
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
  m_total_write_byte[eWriteCounter::FRAME_RATE] = 0;
  m_total_write_byte[eWriteCounter::FRAME_SYNDROME] = 0;
}

void TextOutputIcov::writeBlockSize() {

  //  std::cout << "WRITE SIZE " <<
  //  m_total_write_byte[eWriteCounter::BLOCK_RATE]
  //            << " " << m_total_write_byte[eWriteCounter::BLOCK_SYNDROME]
  //            << std::endl;

  m_total_write_byte[eWriteCounter::FRAME_RATE] +=
      m_total_write_byte[eWriteCounter::BLOCK_RATE];
  m_total_write_byte[eWriteCounter::FRAME_SYNDROME] +=
      m_total_write_byte[eWriteCounter::BLOCK_SYNDROME];

  m_total_write_byte[eWriteCounter::BLOCK_RATE] = 0;
  m_total_write_byte[eWriteCounter::BLOCK_SYNDROME] = 0;
}

void TextOutputIcov::writeBitplaneCount(const uint &v) {
  getChannelNodeJSON()["bitplane_count"] = v;
  getChannelNodeJSON()["stored_syndrome_rate"] = boost::json::array();
}

void TextOutputIcov::writeStoredSyndromeSize(const uint &v) {
  getChannelNodeJSON()["stored_syndrome_rate"].as_array().push_back(v);
}

void TextOutputIcov::writeSIRate(const uint &r) {
  m_total_write_byte[eWriteCounter::BLOCK_RATE] += sizeof(rate_t);
  m_currentPrednodeJSON["syndrome_rate"].as_array().push_back(r);
}

void TextOutputIcov::writeSIMode(const uint &v) {
  m_total_write_byte[eWriteCounter::BLOCK_RATE] += sizeof(bitstream::byte);

  if (v == bitstream::flag_t::UnavailableSI) {
    m_currentPrednodeJSON["available_si_flag"] = false;
  } else {
    m_currentPrednodeJSON["available_si_flag"] = true;
    m_currentPrednodeJSON["mode"] = v;
  }
}

} // namespace encoder
} // namespace icov

void icov::encoder::TextOutputIcov::startWriteBlock(
    const omnidirectional_layer::Block &b) {
  m_blockNodeJSON.clear();
  m_blockNodeJSON["block_id"] = b.getIdSelf();
  m_blockNodeJSON["is_access_block"] = false;
  m_blockNodeJSON["channels"] = boost::json::object();
}

void icov::encoder::TextOutputIcov::startWritePrediction(
    icov::omnidirectional_layer::IBlockPredictionManager::prediction_t pred) {
  m_currentPrednodeJSON.clear();
  auto pred_type = omnidirectional_layer::BlockPrediction::toPredType(pred);
  if (pred_type == omnidirectional_layer::BlockPrediction::ePredType::NONE) {
    m_blockNodeJSON["is_access_block"] = true;
  }
}

void icov::encoder::TextOutputIcov::endWritePrediction(
    omnidirectional_layer::IBlockPredictionManager::prediction_t pred) {
  auto pred_type = omnidirectional_layer::BlockPrediction::toPredType(pred);
  getChannelNodeJSON()["predictions"]
      .as_object()[omnidirectional_layer::BlockPrediction::toString(
          pred_type)] = m_currentPrednodeJSON;
}

void icov::encoder::TextOutputIcov::endWriteBlock(
    const omnidirectional_layer::Block &b) {
  getStream(bitstream::eIcovStream::ENCODE_PARAMS) << m_blockNodeJSON;
}

boost::json::object &icov::encoder::TextOutputIcov::getChannelNodeJSON() {

  if (!m_blockNodeJSON["channels"]
           .as_object()[std::to_string(channel())]
           .is_object()) {

    boost::json::object channelNodeJSON;
    channelNodeJSON["predictions"] = boost::json::object();

    m_blockNodeJSON["channels"].as_object()[std::to_string(channel())] =
        channelNodeJSON;
  }

  return m_blockNodeJSON["channels"]
      .as_object()[std::to_string(channel())]
      .as_object();
}
