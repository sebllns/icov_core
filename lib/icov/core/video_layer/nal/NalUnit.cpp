#include "NalUnit.h"

using namespace icov::bitstream;

void NalUnit::setRbsp(byte *data, int length) {
  m_rbsp = data;
  m_rbsp_length = length;
}

enum_t::NalUnitType NalUnit::getType() const { return m_nalUnitType; }

int NalUnit::getRbspLength() const { return m_rbsp_length; }

const byte *NalUnit::getRbsp() const { return m_rbsp; }

void NalUnit::readRbsp(byte *dst, int length, int offset) const {
  memcpy(dst, m_rbsp + offset, length);
}

byte NalUnit::getRbspByte(int position) const { return m_rbsp[position]; }
void NalUnit::serialize(byte *&dst) const {
  const size_t header_size = 6;
  byte header[header_size];
  header[0] = ((byte)m_forbiddenZeroBit) << 7;
  header[0] |= ((byte)m_nuhReservedZeroBit) << 6;
  header[0] |= ((byte)m_nuhLayerId) & 0b00111111;

  header[1] = (((byte)m_nalUnitType) << 3) & 0b11111000;
  header[1] |= ((byte)m_temporalId) & 0b00000111;

  header[2] = (m_rbsp_length >> 24) & 0xFF;
  header[3] = (m_rbsp_length >> 16) & 0xFF;
  header[4] = (m_rbsp_length >> 8) & 0xFF;
  header[5] = m_rbsp_length & 0xFF;

  dst = new byte[m_rbsp_length + sizeof(header)];

  memcpy(dst, header, sizeof(header));
  memcpy(dst + sizeof(header), m_rbsp, m_rbsp_length);
}

void NalUnit::deserialize(byte *src) {
  const size_t header_size = 6;
  byte header[header_size];
  memcpy(header, src, sizeof(header));

  m_forbiddenZeroBit = (header[0] >> 7) & 0x01;
  m_nuhReservedZeroBit = (header[0] >> 6) & 0x01;
  m_nuhLayerId = header[0] & 0b00111111;

  m_nalUnitType = (enum_t::NalUnitType)((header[1] & 0b11111000) >> 3);
  m_temporalId = header[1] & 0b00000111;

  m_rbsp_length =
      header[2] << 24 | header[3] << 16 | header[4] << 8 | header[5];

  header[2] = (m_rbsp_length >> 24) & 0xFF;
  header[3] = (m_rbsp_length >> 16) & 0xFF;
  header[4] = (m_rbsp_length >> 8) & 0xFF;
  header[5] = m_rbsp_length & 0xFF;

  // m_rbsp = new byte[m_rbsp_length];
  // memcpy(m_rbsp, src + sizeof(header), m_rbsp_length);
  m_rbsp = src + sizeof(header);
}
