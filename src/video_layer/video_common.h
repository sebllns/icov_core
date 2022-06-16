#pragma once

#include "common.h"
#include "CommonLib/Common.h"
#include "CommonLib/TypeDef.h"

using Bool = bool;
using Int = int;
using UInt = unsigned int;
using Void = void;
using Int64 = long long;
using UInt64 = unsigned long long;
using UChar = unsigned char; // Unsigned 8-bit values

//typedef       unsigned int        UInt;
//typedef       int                 Int;
//typedef       unsigned char       UChar; // Unsigned 8-bit values
//typedef       void                Void;
//typedef       bool                Bool;
//typedef       long long           Int64;
//typedef       unsigned long long  UInt64;

static const Int chromaQPMappingTableSize = 58;
extern const UChar  g_aucChromaScale[NUM_CHROMA_FORMAT][chromaQPMappingTableSize];

inline Int getScaledChromaQP(Int unscaledChromaQP, const ChromaFormat chFmt)
{
  return g_aucChromaScale[chFmt][Clip3(0, (chromaQPMappingTableSize - 1), unscaledChromaQP)];
}

static const Int ADJ_QUANT_SHIFT =                                  7;
static const Int ADJ_DEQUANT_SHIFT =            ( ADJ_QUANT_SHIFT + 1 );
