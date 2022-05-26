#pragma one

#include "common.h"
#include "CommonLib/Common.h"
#include "CommonLib/TypeDef.h"

typedef       unsigned int        UInt;
typedef       int                 Int;
typedef       unsigned char       UChar; // Unsigned 8-bit values
typedef       void                Void;
typedef       bool                Bool;
typedef       long long           Int64;
typedef       unsigned long long  UInt64;

static const Int chromaQPMappingTableSize = 58;
extern const UChar  g_aucChromaScale[NUM_CHROMA_FORMAT][chromaQPMappingTableSize];

inline Int getScaledChromaQP(Int unscaledChromaQP, const ChromaFormat chFmt)
{
  return g_aucChromaScale[chFmt][Clip3(0, (chromaQPMappingTableSize - 1), unscaledChromaQP)];
}
