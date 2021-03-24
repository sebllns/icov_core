/** \file     InteractiveCodedUnit.h
    \brief    InteractiveCodedUnit for VCL data
*/

#ifndef INTERACTIVECODEDUNIT_H
#define INTERACTIVECODEDUNIT_H

#include "common.h"

class InteractiveCodedUnit
{
public:
  InteractiveCodedUnit(const icov::byte *encodedData, icov::int8b dataLength) : m_encodedData(encodedData), m_dataLength(dataLength)
  {
  }

  const icov::int8b  GetDataLength() const
  {
    return m_dataLength;
  }

  const icov::byte *GetEncodedData() const
  {
    return m_encodedData;
  }

private:
  const icov::byte *m_encodedData;
  icov::int8b  m_dataLength;
};

#endif // INTERACTIVECODEDUNIT_H
