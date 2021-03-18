/** \file     InteractiveCodedUnit.h
    \brief    InteractiveCodedUnit for VCL data
*/

#ifndef INTERACTIVECODEDUNIT_H
#define INTERACTIVECODEDUNIT_H

#include "common.h"

class InteractiveCodedUnit
{
public:
  InteractiveCodedUnit(const icov::byte *encodedData, uint8_t dataLength) : m_encodedData(encodedData), m_dataLength(dataLength)
  {
  }

  const uint8_t GetDataLength() const
  {
    return m_dataLength;
  }

  const icov::byte *GetEncodedData() const
  {
    return m_encodedData;
  }

private:
  const icov::byte *m_encodedData;
  uint8_t m_dataLength;
};

#endif // INTERACTIVECODEDUNIT_H
