/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/
#ifndef TRANSFORM2D_H
#define TRANSFORM2D_H
#include "CommonLib/CommonDef.h"
#include "CommonLib/Unit.h"
#include "video_common.h"

using namespace icov::video;

class MyTransform2D
{
protected:
    TCoeff*  m_plTempCoeff;

public:
    MyTransform2D();
    ~MyTransform2D();

    // forward Transform
    Void transformNxN(CoeffBuf& transformCoeffBuff, const PelUnitBuf &resiUnitBuf, const CompArea &area, const Int bitDepths, const int maxLog2TrDynamicRange =15 );
    Void invTransformNxN(PelUnitBuf &resiUnitBuf, const CompArea &area, const CCoeffBuf& transformCoeffBuff, const Int bitDepths, const int maxLog2TrDynamicRange =15 );
};

#endif // TRANSFORM2D_H
