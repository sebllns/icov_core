#include "TextOutputIcov.h"
#include "encoder/OutputIcovBase.h"
#include "icov_io.h"
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
#include <iterator>
#include <limits>
#include <memory>
#include <string>
namespace icov {
namespace encoder {

TextOutputIcov::TextOutputIcov(const boost::filesystem::path &path)
    : OutputIcovBase(path, 0) {

  m_blockNodeJSON.initChannelNode = []() {
    boost::json::object channelNodeJSON;
    channelNodeJSON["predictions"] = boost::json::object();
    return channelNodeJSON;
  };

  m_syndromeNodeJSON.initChannelNode = []() {
    boost::json::object channelNodeJSON;
    return channelNodeJSON;
  };
}

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
    m_currentPrednodeJSON["syndrome_numcode"] = boost::json::array();
    m_currentPrednodeJSON["syndrome_rate"] = boost::json::array();
    m_currentPrednodeJSON["syndrome_bitlength"] = boost::json::array();
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
  m_total_write_byte[eWriteCounter::ENC_BLOCK_SYNDROME_SIZE] += byte_size;

  if (byte_size == 0) {
    m_syndromeNodeJSON.getChannelNodeJSON(channel())["syndrome"] = "";
  } else {
    icov::byte *bytes = new icov::byte[byte_size];
    bitstream::write_bits(s, bytes);

    m_syndromeNodeJSON.getChannelNodeJSON(channel())["syndrome"] =
        icov::io::make_hex_string(bytes, bytes + byte_size, true, true);

    delete[] bytes;
  }
}

void TextOutputIcov::writeFrameSize() {
  m_total_write_byte[eWriteCounter::ENC_FRAME_RATE_SIZE] = 0;
  m_total_write_byte[eWriteCounter::ENC_FRAME_SYNDROME_SIZE] = 0;
}

void TextOutputIcov::writeBlockSize() {

  //  std::cout << "WRITE SIZE " <<
  //  m_total_write_byte[eWriteCounter::BLOCK_RATE]
  //            << " " << m_total_write_byte[eWriteCounter::BLOCK_SYNDROME]
  //            << std::endl;
  
  m_total_write_byte[eWriteCounter::ENC_FRAME_RATE_SIZE] +=
      m_total_write_byte[eWriteCounter::ENC_BLOCK_RATE_SIZE];
  m_total_write_byte[eWriteCounter::ENC_FRAME_SYNDROME_SIZE] +=
      m_total_write_byte[eWriteCounter::ENC_BLOCK_SYNDROME_SIZE];
  
  m_total_write_byte[eWriteCounter::ENC_BLOCK_RATE_SIZE] = 0;
  m_total_write_byte[eWriteCounter::ENC_BLOCK_SYNDROME_SIZE] = 0;
}

void TextOutputIcov::writeBitplaneCount(const uint &v) {
  m_blockNodeJSON.getChannelNodeJSON(channel())["bitplane_count"] = v;
  m_blockNodeJSON.getChannelNodeJSON(channel())["stored_syndrome_rate"] =
      boost::json::array();
  m_blockNodeJSON.getChannelNodeJSON(channel())["stored_syndrome_numcode"] =
      boost::json::array();
  m_blockNodeJSON.getChannelNodeJSON(channel())["stored_syndrome_bitlength"] =
      boost::json::array();
}

void TextOutputIcov::writeStoredSyndromeSize(const NumcodeIdx &v, const Rate &r,
                                             const uint &m) {
  m_blockNodeJSON.getChannelNodeJSON(channel())["stored_syndrome_rate"]
      .as_array()
      .push_back(r);
  m_blockNodeJSON.getChannelNodeJSON(channel())["stored_syndrome_numcode"]
      .as_array()
      .push_back(v);
  m_blockNodeJSON.getChannelNodeJSON(channel())["stored_syndrome_bitlength"]
      .as_array()
      .push_back(m);
}

void TextOutputIcov::writeSIRate(const NumcodeIdx &n, const Rate &r,
                                 const uint &m) {
  m_total_write_byte[eWriteCounter::ENC_BLOCK_RATE_SIZE] += sizeof(rate_t);
  m_currentPrednodeJSON["syndrome_numcode"].as_array().push_back(
      std::to_string(n).c_str());
  m_currentPrednodeJSON["syndrome_rate"].as_array().push_back(
      std::to_string(r).c_str());
  m_currentPrednodeJSON["syndrome_bitlength"].as_array().push_back(
      std::to_string(m).c_str());
}

void TextOutputIcov::writeSIMode(const uint &v) {
  m_total_write_byte[eWriteCounter::ENC_BLOCK_RATE_SIZE] += sizeof(byte);

  if (v == bitstream::flag_t::UnavailableSI) {
    m_currentPrednodeJSON["available_si_flag"] = false;
  } else {
    m_currentPrednodeJSON["available_si_flag"] = true;
    m_currentPrednodeJSON["mode"] = v;
  }
}

} // namespace encoder
} // namespace icov

// void icov::encoder::TextOutputIcov::_initNode(
//    boost::json::object &node, const omnidirectional_layer::Block &b) {
//  node.clear();
//  node["block_id"] = b.getIdSelf();
//  node["is_access_block"] = false;
//  node["channels"] = boost::json::object();
//}

void icov::encoder::TextOutputIcov::startWriteBlock(
    const omnidirectional_layer::Block &b) {
  m_blockNodeJSON.init(b);
  m_syndromeNodeJSON.init(b);
}

void icov::encoder::TextOutputIcov::startWritePrediction(
    icov::omnidirectional_layer::IBlockPredictionManager::prediction_t pred) {
  m_currentPrednodeJSON.clear();
  auto pred_type = omnidirectional_layer::BlockPrediction::toPredType(pred);
  if (pred_type == omnidirectional_layer::BlockPrediction::ePredType::NONE) {
    m_blockNodeJSON.nodeJSON["is_access_block"] = true;
  }
}

void icov::encoder::TextOutputIcov::endWritePrediction(
    omnidirectional_layer::IBlockPredictionManager::prediction_t pred) {
  auto pred_type = omnidirectional_layer::BlockPrediction::toPredType(pred);
  m_blockNodeJSON.getChannelNodeJSON(channel())["predictions"]
      .as_object()[omnidirectional_layer::BlockPrediction::toString(
          pred_type)] = m_currentPrednodeJSON;
}

void icov::encoder::TextOutputIcov::endWriteBlock(
    const omnidirectional_layer::Block &b) {
  getStream(bitstream::eIcovStream::ENCODE_PARAMS) << m_blockNodeJSON.nodeJSON;
  getStream(bitstream::eIcovStream::SYNDROME) << m_syndromeNodeJSON.nodeJSON;
}

// boost::json::object &icov::encoder::TextOutputIcov::getChannelNodeJSON() {

//  if (!m_blockNodeJSON["channels"]
//           .as_object()[std::to_string(channel())]
//           .is_object()) {

//    boost::json::object channelNodeJSON;
//    channelNodeJSON["predictions"] = boost::json::object();

//    m_blockNodeJSON["channels"].as_object()[std::to_string(channel())] =
//        channelNodeJSON;
//  }

//  return m_blockNodeJSON["channels"]
//      .as_object()[std::to_string(channel())]
//      .as_object();
//}


void icov::encoder::TextOutputIcov::writeMse(const float &mse)
{
}
