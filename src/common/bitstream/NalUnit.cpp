#include "NalUnit.h"



void NalUnit::setRbsp(const icov::byte* data, int length) 
{
    m_rbsp = data;
    m_rbsp_length = length;
}

icov::NalUnitType NalUnit::getType() const
{
    return m_nalUnitType;
}

int NalUnit::getRbspLength() const
{
    return m_rbsp_length;
}

const void NalUnit::readRbsp(icov::byte* dst, int length, int offset) const
{
    memcpy(dst, m_rbsp + offset, length);
}

const icov::byte NalUnit::getRbspByte(int position) const
{
    return m_rbsp[position];
}