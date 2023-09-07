#include "BitplaneDecoder.h"
#include "FastArray.h"
#include "Unit.h"

#include "icov_log.h"
#include "ldpc_codec_layer/BitSymbolProvider.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "video_layer/ExtractedStreamDebugger.h"
#include "video_layer/ICOV_Quant.h"
#include "video_layer/ICOV_Transform2d.h"
#include "video_layer/extracted_stream.h"
#include "video_layer/icov_stream.h"
#include "video_layer/vtm_utility.h"
#include <algorithm>
#include <boost/algorithm/algorithm.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <iostream>

namespace icov {
namespace decoder {

namespace fs = boost::filesystem;
using namespace icov::decoder;
using SI_SIDE = omnidirectional_layer::BlockPrediction::ePredType;

BitplaneDecoder::BitplaneDecoder(const params &p, const LdpcCoderParameters &LDPC_params,
                                 const std::shared_ptr<icov::video::BufGroups> &vvc_bufs,
                                 const std::shared_ptr<video::EncodingStructure> &encodingStruct,
                                 const bitstream::sp_IcovExtractedBitstream &extraction)
    : m_params(p), m_vvc_bufs(vvc_bufs), m_encodingStruct(encodingStruct),
      //      m_blockArea(
      //          UnitArea(p.QpParam.chromaFomat, Area(0, 0, p.icu_size,
      //          p.icu_size))
      //              .block(p.CompID)),
      m_ldpc_coder(ldpc_codec_layer::LdpcCoderFactory::create_coder(LDPC_params)),
      m_blockBitplane(m_ldpc_coder->get_n(), 0), m_blockSI(m_ldpc_coder->get_n(), 0),
      m_syndrome(m_ldpc_coder->get_m_max()), m_bsp(m_ldpc_coder->get_n()),
      m_block_prediction(omnidirectional_layer::BlockPrediction::ePredType::UNKNOWN), m_bitstream(extraction),
      m_blockSymbols(m_ldpc_coder->get_n(), 0), m_rate(0)
{
    m_intraPred.init(m_params.QpParam.chromaFomat);
}

BitplaneDecoder::~BitplaneDecoder()
{
}

void icov::decoder::BitplaneDecoder::_decode_bitplane(
    int i, omnidirectional_layer::BlockPrediction::ePredType block_prediction)
{

    byte quantizedNumcode = m_numcodes.front();
    m_numcodes.pop();

    int syndrome_size = bitstream::inverseQuantizedSize(quantizedNumcode, *m_ldpc_coder);

    read_syndrome(syndrome_size);
    decode_bitplane_with_ldpc(i, static_cast<icov::NumcodeIdx>(quantizedNumcode), m_syndrome);

    m_rate += m_ldpc_coder->get_r(syndrome_size, m_ldpc_coder->get_n());
}

void icov::decoder::BitplaneDecoder::_extract_numcode(
    int bp, omnidirectional_layer::BlockPrediction::ePredType block_prediction)
{

    byte quantizedNumcode = (block_prediction == omnidirectional_layer::BlockPrediction::ePredType::NONE)
                                ? bitstream::enum_t::SyndromeType::SyndromeType_SOURCE
                                : m_bitstream->pop();

    bitstream::VideoStreamDebugger::get_const_instance().assertNumcodeEqual(bp, quantizedNumcode);

    m_numcodes.push(quantizedNumcode);
}

void BitplaneDecoder::read_syndrome(int syndrome_size)
{
  m_syndrome.resize(syndrome_size);
  byte bitstream_value = 0;
  ICOV_TRACE << "READ SYNDROME " << syndrome_size;
  for (int i = 0; i < syndrome_size; ++i) {
    if ((i % BYTE_SIZE) == 0) {
      bitstream_value = m_bitstream->pop();
      // std::cout << " " << (int)bitstream_value;
    }
    m_syndrome[i] =
        ((bitstream_value & (1 << (BYTE_SIZE - 1 - i % BYTE_SIZE))) > 0);
  }
}

void BitplaneDecoder::decode_bitplane_with_ldpc(int bp, NumcodeIdx numcode, const Syndrome &synd)
{
  if (m_block_prediction ==
      icov::omnidirectional_layer::BlockPrediction::ePredType::NONE) {
    ICOV_ASSERT_MSG(numcode ==
                        bitstream::enum_t::SyndromeType::SyndromeType_SOURCE,
                    "Numcode value must be set to 1 for access block");
  }

  bitstream::VideoStreamDebugger::get_const_instance().assertSyndromeEqual(
      bp, synd);

  m_ldpc_coder->decode_bitplane(m_blockBitplane, bp, m_bsp, numcode, synd);

  bitstream::VideoStreamDebugger::get_const_instance().assertBitplaneEqual(
      bp, m_bsp.src_bits().at(bp));

  auto mp_icu_enc = m_encodingStruct->getICU(m_icuIndex);

  if ((bp + 1) == m_bsp.bitplane_count()) {
    icov::video::TransformUnit &dec_tu = mp_icu_enc.sourceX_TU(0);
    std::copy(m_bsp.src_symboles().begin(), m_bsp.src_symboles().end(),
              dec_tu.getQuantizedTransformCoeffs(m_params.CompID).buf);
    //    icov::io::write_vector(std::cout, m_bsp.src_symboles(), ' ');
    //    std::cout << std::endl;
  }
}


icov::Syndrome &BitplaneDecoder::syndrome()
{
  return m_syndrome;
}

Rate BitplaneDecoder::rate() const
{
  return m_rate;
}
} // namespace decoder
} // namespace icov
