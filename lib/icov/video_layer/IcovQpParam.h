/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/

#pragma once

#include "video_common.h"


namespace icov {
namespace video {

/// QP struct
/// cf. CommonLib/Quant.h:61
//#include "CommonLib/Quant.h"
struct IcovQpParam
{
    int Qp;
    int per;
    int rem;

    ChannelType channelType;
    ChromaFormat chromaFomat;

    IcovQpParam(const int           qpy,
              const ChannelType   chType,
              const ChromaFormat  chFmt,
              const int channelBitDepth,
              const int           chromaQPOffset=0,
              const int           dqp=0 );

}; // END STRUCT DEFINITION ICOV_QpParam

}
}
