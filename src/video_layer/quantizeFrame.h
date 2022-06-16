/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/
#pragma once

#include <CommonLib/Buffer.h>
#include "encodingstructure.h"

void quantizeFrame(const int qp, const PelUnitBuf &originalImage, PelUnitBuf &quantizedImage, EncodingStructure &encStruct, const int bitDepths[MAX_NUM_CHANNEL_TYPE], int maxLog2TrDynamicRange);

void quantizeFrame(const int qp, const PelUnitBuf &originalImage, PelUnitBuf &quantizedImage, EncodingStructure &encStruct, const ComponentID compID, const int bitDepths[MAX_NUM_CHANNEL_TYPE], int maxLog2TrDynamicRange);
