/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/
#include "mydistorsioncost.h"

FpDistFunc MyDistortionCost::m_afpDistortFunc[DF_TOTAL_FUNCTIONS] = { nullptr, };

MyDistortionCost::MyDistortionCost()
{
    init();

    // I did it by myself just to make sure that the code is working. it is because I gave weight 1 to chromas also. for more information look at MyDistortionCost::getDistPart. If you want VVC weighting, call setDistortionChromaWeight(const ComponentID compID, const Int qpy, const ChromaFormat chFmt, const Int chromaQPOffset) by yourself
    setDistortionChromaWeight(COMPONENT_Cb, 1);
    setDistortionChromaWeight(COMPONENT_Cr, 1);
}

MyDistortionCost::~MyDistortionCost()
{

}

Distortion MyDistortionCost::getDistPart(const CPelBuf &org, const CPelBuf &cur, Int bitDepth, const ComponentID compID, DFunc eDFunc)
{
    DistParam cDtParam;

//    cDtParam.isQtbt     = m_useQtbt;  // TODO: it is used only for HAD (RdCost::xGetHADs or RdCost::xGetHADs_SIMD)
    cDtParam.org        = org;
    cDtParam.cur        = cur;
    cDtParam.step       = 1;
    cDtParam.bitDepth   = bitDepth;
    cDtParam.compID     = compID;

    if( isPowerOf2( org.width ) )
    {
      cDtParam.distFunc = m_afpDistortFunc[eDFunc + g_aucLog2[org.width]];
    }
    else
    {
      cDtParam.distFunc = m_afpDistortFunc[eDFunc];
    }

    if (isChroma(compID))
    {
      return ((Distortion) (m_distortionWeight[ MAP_CHROMA(compID) ] * cDtParam.distFunc( cDtParam )));
    }
    else
    {
      return cDtParam.distFunc( cDtParam );
    }
}

Void MyDistortionCost::setDistortionChromaWeight(const ComponentID compID, const Int qpy, const ChromaFormat chFmt, const Int chromaQPOffset)
{
    CHECK(toChannelType(compID)!=CHANNEL_TYPE_CHROMA, "only chroma values are used for weights")
    // took it from EncSlice::setUpLambda. If you look more carefully, it is also consistent with MyQpParam::MyQpParam

    Int qpc                  = ( qpy + chromaQPOffset < 0 ) ? qpy : getScaledChromaQP( qpy + chromaQPOffset, chFmt );
    Double tmpWeight         = pow( 2.0, ( qpy - qpc ) / 3.0 );  // takes into account of the chroma qp mapping and chroma qp Offset
    setDistortionChromaWeight( compID, tmpWeight );
}


Void MyDistortionCost::init()
{
    m_afpDistortFunc[DF_SSE    ] = MyDistortionCost::xGetSSE;
    m_afpDistortFunc[DF_SSE2   ] = MyDistortionCost::xGetSSE;
    m_afpDistortFunc[DF_SSE4   ] = MyDistortionCost::xGetSSE4;
    m_afpDistortFunc[DF_SSE8   ] = MyDistortionCost::xGetSSE8;
    m_afpDistortFunc[DF_SSE16  ] = MyDistortionCost::xGetSSE16;
    m_afpDistortFunc[DF_SSE32  ] = MyDistortionCost::xGetSSE32;
    m_afpDistortFunc[DF_SSE64  ] = MyDistortionCost::xGetSSE64;
    m_afpDistortFunc[DF_SSE16N ] = MyDistortionCost::xGetSSE16N;

    m_afpDistortFunc[DF_SAD    ] = MyDistortionCost::xGetSAD;
    m_afpDistortFunc[DF_SAD2   ] = MyDistortionCost::xGetSAD;
    m_afpDistortFunc[DF_SAD4   ] = MyDistortionCost::xGetSAD4;
    m_afpDistortFunc[DF_SAD8   ] = MyDistortionCost::xGetSAD8;
    m_afpDistortFunc[DF_SAD16  ] = MyDistortionCost::xGetSAD16;
    m_afpDistortFunc[DF_SAD32  ] = MyDistortionCost::xGetSAD32;
    m_afpDistortFunc[DF_SAD64  ] = MyDistortionCost::xGetSAD64;
    m_afpDistortFunc[DF_SAD16N ] = MyDistortionCost::xGetSAD16N;

    m_afpDistortFunc[DF_SAD12  ] = MyDistortionCost::xGetSAD12;
    m_afpDistortFunc[DF_SAD24  ] = MyDistortionCost::xGetSAD24;
    m_afpDistortFunc[DF_SAD48  ] = MyDistortionCost::xGetSAD48;

//    m_afpDistortFunc[DF_HAD    ] = MyDistortionCost::xGetHADs;
//    m_afpDistortFunc[DF_HAD2   ] = MyDistortionCost::xGetHADs;
//    m_afpDistortFunc[DF_HAD4   ] = MyDistortionCost::xGetHADs;
//    m_afpDistortFunc[DF_HAD8   ] = MyDistortionCost::xGetHADs;
//    m_afpDistortFunc[DF_HAD16  ] = MyDistortionCost::xGetHADs;
//    m_afpDistortFunc[DF_HAD32  ] = MyDistortionCost::xGetHADs;
//    m_afpDistortFunc[DF_HAD64  ] = MyDistortionCost::xGetHADs;
//    m_afpDistortFunc[DF_HAD16N ] = MyDistortionCost::xGetHADs;

//    m_afpDistortFunc[DF_MRSAD    ] = MyDistortionCost::xGetMRSAD;
//    m_afpDistortFunc[DF_MRSAD2   ] = MyDistortionCost::xGetMRSAD;
//    m_afpDistortFunc[DF_MRSAD4   ] = MyDistortionCost::xGetMRSAD4;
//    m_afpDistortFunc[DF_MRSAD8   ] = MyDistortionCost::xGetMRSAD8;
//    m_afpDistortFunc[DF_MRSAD16  ] = MyDistortionCost::xGetMRSAD16;
//    m_afpDistortFunc[DF_MRSAD32  ] = MyDistortionCost::xGetMRSAD32;
//    m_afpDistortFunc[DF_MRSAD64  ] = MyDistortionCost::xGetMRSAD64;
//    m_afpDistortFunc[DF_MRSAD16N ] = MyDistortionCost::xGetMRSAD16N;

//    m_afpDistortFunc[DF_MRSAD12  ] = MyDistortionCost::xGetMRSAD12;
//    m_afpDistortFunc[DF_MRSAD24  ] = MyDistortionCost::xGetMRSAD24;
//    m_afpDistortFunc[DF_MRSAD48  ] = MyDistortionCost::xGetMRSAD48;

//    m_afpDistortFunc[DF_MRHAD    ] = MyDistortionCost::xGetMRHADs;
//    m_afpDistortFunc[DF_MRHAD2   ] = MyDistortionCost::xGetMRHADs;
//    m_afpDistortFunc[DF_MRHAD4   ] = MyDistortionCost::xGetMRHADs;
//    m_afpDistortFunc[DF_MRHAD8   ] = MyDistortionCost::xGetMRHADs;
//    m_afpDistortFunc[DF_MRHAD16  ] = MyDistortionCost::xGetMRHADs;
//    m_afpDistortFunc[DF_MRHAD32  ] = MyDistortionCost::xGetMRHADs;
//    m_afpDistortFunc[DF_MRHAD64  ] = MyDistortionCost::xGetMRHADs;
//    m_afpDistortFunc[DF_MRHAD16N ] = MyDistortionCost::xGetMRHADs;

//    m_afpDistortFunc[DF_SAD_FULL_NBIT   ] = MyDistortionCost::xGetSAD_full;
//    m_afpDistortFunc[DF_SAD_FULL_NBIT2  ] = MyDistortionCost::xGetSAD_full;
//    m_afpDistortFunc[DF_SAD_FULL_NBIT4  ] = MyDistortionCost::xGetSAD_full;
//    m_afpDistortFunc[DF_SAD_FULL_NBIT8  ] = MyDistortionCost::xGetSAD_full;
//    m_afpDistortFunc[DF_SAD_FULL_NBIT16 ] = MyDistortionCost::xGetSAD_full;
//    m_afpDistortFunc[DF_SAD_FULL_NBIT32 ] = MyDistortionCost::xGetSAD_full;
//    m_afpDistortFunc[DF_SAD_FULL_NBIT64 ] = MyDistortionCost::xGetSAD_full;
//    m_afpDistortFunc[DF_SAD_FULL_NBIT16N] = MyDistortionCost::xGetSAD_full;

//    initRdCostX86();
}


Void MyDistortionCost::initRdCostX86()
{

}

Distortion MyDistortionCost::xGetSSEw(const DistParam &rcDtParam)
{
    const Pel            *piOrg      = rcDtParam.org.buf;
    const Pel            *piCur      = rcDtParam.cur.buf;
    const Int             iCols      = rcDtParam.org.width;
    const Int             iStrideCur = rcDtParam.cur.stride;
    const Int             iStrideOrg = rcDtParam.org.stride;
    const ComponentID     compID     = rcDtParam.compID;

    CHECK( rcDtParam.subShift != 0, "Subshift not supported" ); // NOTE: what is this protecting?
    CHECK( compID >= MAX_NUM_COMPONENT, "Invalid channel" );

    const WPScalingParam &wpCur           = rcDtParam.wpCur[compID];
    const Int             w0              = wpCur.w;
    const Int             offset          = wpCur.offset;
    const Int             shift           = wpCur.shift;
    const Int             round           = wpCur.round;
    const UInt            distortionShift = DISTORTION_PRECISION_ADJUSTMENT((rcDtParam.bitDepth-8) << 1);

    Distortion sum = 0;

    if (rcDtParam.isBiPred)
    {
      for(Int iRows = rcDtParam.org.height ; iRows != 0; iRows-- )
      {
        for (Int n = 0; n < iCols; n++ )
        {
          const Pel pred     = ( (w0*piCur[n] + round) >> shift ) + offset ;
          const Pel residual = piOrg[n] - pred;
          sum += ( Distortion(residual) * Distortion(residual) ) >> distortionShift;
        }
        piOrg += iStrideOrg;
        piCur += iStrideCur;
      }
    }
    else
    {
      ClpRng clpRng;  // this just affects the cost - fix this later
      clpRng.min = 0;
      clpRng.max = (1 << rcDtParam.bitDepth) - 1;

      for(Int iRows = rcDtParam.org.height; iRows != 0; iRows-- )
      {
        for (Int n = 0; n < iCols; n++ )
        {
          const Pel pred     = ClipPel(( (w0*piCur[n] + round) >> shift ) + offset, clpRng) ;
          const Pel residual = piOrg[n] - pred;
          sum += ( Distortion(residual) * Distortion(residual) ) >> distortionShift;
        }
        piOrg += iStrideOrg;
        piCur += iStrideCur;
      }
    }

    return sum;
}


Distortion MyDistortionCost::xGetSSE( const DistParam &rcDtParam )
{
  if ( rcDtParam.applyWeight )
  {
    return RdCostWeightPrediction::xGetSSEw( rcDtParam );
  }

  const Pel* piOrg      = rcDtParam.org.buf;
  const Pel* piCur      = rcDtParam.cur.buf;
  Int  iRows            = rcDtParam.org.height;
  Int  iCols            = rcDtParam.org.width;
  Int  iStrideCur       = rcDtParam.cur.stride;
  Int  iStrideOrg       = rcDtParam.org.stride;

  Distortion uiSum   = 0;
  UInt       uiShift = DISTORTION_PRECISION_ADJUSTMENT((rcDtParam.bitDepth-8) << 1);

  Intermediate_Int iTemp;

  for( ; iRows != 0; iRows-- )
  {
    for (Int n = 0; n < iCols; n++ )
    {
      iTemp = piOrg[n  ] - piCur[n  ];
      uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    }
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum );
}

Distortion MyDistortionCost::xGetSSE4( const DistParam &rcDtParam )
{
  if ( rcDtParam.applyWeight )
  {
    CHECK( rcDtParam.org.width != 4, "Invalid size" );
    return RdCostWeightPrediction::xGetSSEw( rcDtParam );
  }

  const Pel* piOrg   = rcDtParam.org.buf;
  const Pel* piCur   = rcDtParam.cur.buf;
  Int  iRows         = rcDtParam.org.height;
  Int  iStrideOrg    = rcDtParam.org.stride;
  Int  iStrideCur    = rcDtParam.cur.stride;

  Distortion uiSum   = 0;
  UInt       uiShift = DISTORTION_PRECISION_ADJUSTMENT((rcDtParam.bitDepth-8) << 1);

  Intermediate_Int  iTemp;

  for( ; iRows != 0; iRows-- )
  {

    iTemp = piOrg[0] - piCur[0]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[1] - piCur[1]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[2] - piCur[2]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[3] - piCur[3]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum );
}

Distortion MyDistortionCost::xGetSSE8( const DistParam &rcDtParam )
{
  if ( rcDtParam.applyWeight )
  {
    CHECK( rcDtParam.org.width != 8, "Invalid size" );
    return RdCostWeightPrediction::xGetSSEw( rcDtParam );
  }

  const Pel* piOrg   = rcDtParam.org.buf;
  const Pel* piCur   = rcDtParam.cur.buf;
  Int  iRows         = rcDtParam.org.height;
  Int  iStrideOrg    = rcDtParam.org.stride;
  Int  iStrideCur    = rcDtParam.cur.stride;

  Distortion uiSum   = 0;
  UInt       uiShift = DISTORTION_PRECISION_ADJUSTMENT((rcDtParam.bitDepth-8) << 1);

  Intermediate_Int  iTemp;

  for( ; iRows != 0; iRows-- )
  {
    iTemp = piOrg[0] - piCur[0]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[1] - piCur[1]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[2] - piCur[2]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[3] - piCur[3]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[4] - piCur[4]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[5] - piCur[5]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[6] - piCur[6]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[7] - piCur[7]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum );
}

Distortion MyDistortionCost::xGetSSE16( const DistParam &rcDtParam )
{
  if ( rcDtParam.applyWeight )
  {
    CHECK( rcDtParam.org.width != 16, "Invalid size" );
    return RdCostWeightPrediction::xGetSSEw( rcDtParam );
  }

  const Pel* piOrg   = rcDtParam.org.buf;
  const Pel* piCur   = rcDtParam.cur.buf;
  Int  iRows         = rcDtParam.org.height;
  Int  iStrideOrg    = rcDtParam.org.stride;
  Int  iStrideCur    = rcDtParam.cur.stride;

  Distortion uiSum   = 0;
  UInt       uiShift = DISTORTION_PRECISION_ADJUSTMENT((rcDtParam.bitDepth-8) << 1);

  Intermediate_Int  iTemp;

  for( ; iRows != 0; iRows-- )
  {

    iTemp = piOrg[ 0] - piCur[ 0]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 1] - piCur[ 1]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 2] - piCur[ 2]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 3] - piCur[ 3]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 4] - piCur[ 4]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 5] - piCur[ 5]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 6] - piCur[ 6]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 7] - piCur[ 7]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 8] - piCur[ 8]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 9] - piCur[ 9]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[10] - piCur[10]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[11] - piCur[11]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[12] - piCur[12]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[13] - piCur[13]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[14] - piCur[14]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[15] - piCur[15]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum );
}

Distortion MyDistortionCost::xGetSSE16N( const DistParam &rcDtParam )
{
  if ( rcDtParam.applyWeight )
  {
    return RdCostWeightPrediction::xGetSSEw( rcDtParam );
  }
  const Pel* piOrg   = rcDtParam.org.buf;
  const Pel* piCur   = rcDtParam.cur.buf;
  Int  iRows         = rcDtParam.org.height;
  Int  iCols         = rcDtParam.org.width;
  Int  iStrideOrg    = rcDtParam.org.stride;
  Int  iStrideCur    = rcDtParam.cur.stride;

  Distortion uiSum   = 0;
  UInt       uiShift = DISTORTION_PRECISION_ADJUSTMENT((rcDtParam.bitDepth-8) << 1);

  Intermediate_Int  iTemp;

  for( ; iRows != 0; iRows-- )
  {
    for (Int n = 0; n < iCols; n+=16 )
    {

      iTemp = piOrg[n+ 0] - piCur[n+ 0]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
      iTemp = piOrg[n+ 1] - piCur[n+ 1]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
      iTemp = piOrg[n+ 2] - piCur[n+ 2]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
      iTemp = piOrg[n+ 3] - piCur[n+ 3]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
      iTemp = piOrg[n+ 4] - piCur[n+ 4]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
      iTemp = piOrg[n+ 5] - piCur[n+ 5]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
      iTemp = piOrg[n+ 6] - piCur[n+ 6]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
      iTemp = piOrg[n+ 7] - piCur[n+ 7]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
      iTemp = piOrg[n+ 8] - piCur[n+ 8]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
      iTemp = piOrg[n+ 9] - piCur[n+ 9]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
      iTemp = piOrg[n+10] - piCur[n+10]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
      iTemp = piOrg[n+11] - piCur[n+11]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
      iTemp = piOrg[n+12] - piCur[n+12]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
      iTemp = piOrg[n+13] - piCur[n+13]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
      iTemp = piOrg[n+14] - piCur[n+14]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
      iTemp = piOrg[n+15] - piCur[n+15]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);

    }
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum );
}

Distortion MyDistortionCost::xGetSSE32( const DistParam &rcDtParam )
{
  if ( rcDtParam.applyWeight )
  {
    CHECK( rcDtParam.org.width != 32, "Invalid size" );
    return RdCostWeightPrediction::xGetSSEw( rcDtParam );
  }

  const Pel* piOrg   = rcDtParam.org.buf;
  const Pel* piCur   = rcDtParam.cur.buf;
  Int  iRows         = rcDtParam.org.height;
  Int  iStrideOrg    = rcDtParam.org.stride;
  Int  iStrideCur    = rcDtParam.cur.stride;

  Distortion uiSum   = 0;
  UInt       uiShift = DISTORTION_PRECISION_ADJUSTMENT((rcDtParam.bitDepth-8) << 1);

  Intermediate_Int  iTemp;

  for( ; iRows != 0; iRows-- )
  {

    iTemp = piOrg[ 0] - piCur[ 0]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 1] - piCur[ 1]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 2] - piCur[ 2]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 3] - piCur[ 3]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 4] - piCur[ 4]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 5] - piCur[ 5]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 6] - piCur[ 6]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 7] - piCur[ 7]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 8] - piCur[ 8]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 9] - piCur[ 9]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[10] - piCur[10]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[11] - piCur[11]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[12] - piCur[12]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[13] - piCur[13]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[14] - piCur[14]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[15] - piCur[15]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[16] - piCur[16]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[17] - piCur[17]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[18] - piCur[18]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[19] - piCur[19]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[20] - piCur[20]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[21] - piCur[21]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[22] - piCur[22]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[23] - piCur[23]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[24] - piCur[24]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[25] - piCur[25]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[26] - piCur[26]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[27] - piCur[27]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[28] - piCur[28]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[29] - piCur[29]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[30] - piCur[30]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[31] - piCur[31]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum );
}

Distortion MyDistortionCost::xGetSSE64( const DistParam &rcDtParam )
{
  if ( rcDtParam.applyWeight )
  {
    CHECK( rcDtParam.org.width != 64, "Invalid size" );
    return RdCostWeightPrediction::xGetSSEw( rcDtParam );
  }

  const Pel* piOrg   = rcDtParam.org.buf;
  const Pel* piCur   = rcDtParam.cur.buf;
  Int  iRows         = rcDtParam.org.height;
  Int  iStrideOrg    = rcDtParam.org.stride;
  Int  iStrideCur    = rcDtParam.cur.stride;

  Distortion uiSum   = 0;
  UInt       uiShift = DISTORTION_PRECISION_ADJUSTMENT((rcDtParam.bitDepth-8) << 1);

  Intermediate_Int  iTemp;

  for( ; iRows != 0; iRows-- )
  {
    iTemp = piOrg[ 0] - piCur[ 0]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 1] - piCur[ 1]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 2] - piCur[ 2]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 3] - piCur[ 3]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 4] - piCur[ 4]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 5] - piCur[ 5]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 6] - piCur[ 6]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 7] - piCur[ 7]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 8] - piCur[ 8]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[ 9] - piCur[ 9]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[10] - piCur[10]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[11] - piCur[11]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[12] - piCur[12]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[13] - piCur[13]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[14] - piCur[14]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[15] - piCur[15]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[16] - piCur[16]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[17] - piCur[17]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[18] - piCur[18]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[19] - piCur[19]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[20] - piCur[20]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[21] - piCur[21]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[22] - piCur[22]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[23] - piCur[23]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[24] - piCur[24]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[25] - piCur[25]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[26] - piCur[26]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[27] - piCur[27]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[28] - piCur[28]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[29] - piCur[29]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[30] - piCur[30]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[31] - piCur[31]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[32] - piCur[32]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[33] - piCur[33]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[34] - piCur[34]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[35] - piCur[35]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[36] - piCur[36]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[37] - piCur[37]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[38] - piCur[38]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[39] - piCur[39]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[40] - piCur[40]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[41] - piCur[41]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[42] - piCur[42]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[43] - piCur[43]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[44] - piCur[44]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[45] - piCur[45]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[46] - piCur[46]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[47] - piCur[47]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[48] - piCur[48]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[49] - piCur[49]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[50] - piCur[50]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[51] - piCur[51]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[52] - piCur[52]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[53] - piCur[53]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[54] - piCur[54]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[55] - piCur[55]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[56] - piCur[56]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[57] - piCur[57]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[58] - piCur[58]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[59] - piCur[59]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[60] - piCur[60]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[61] - piCur[61]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[62] - piCur[62]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);
    iTemp = piOrg[63] - piCur[63]; uiSum += Distortion(( iTemp * iTemp ) >> uiShift);

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum );
}



Distortion MyDistortionCost::xGetSAD( const DistParam& rcDtParam )
{
  if ( rcDtParam.applyWeight )
  {
    return RdCostWeightPrediction::xGetSADw( rcDtParam );
  }

  const Pel* piOrg           = rcDtParam.org.buf;
  const Pel* piCur           = rcDtParam.cur.buf;
  const Int  iCols           = rcDtParam.org.width;
        Int  iRows           = rcDtParam.org.height;
  const Int  iSubShift       = rcDtParam.subShift;
  const Int  iSubStep        = ( 1 << iSubShift );
  const Int  iStrideCur      = rcDtParam.cur.stride * iSubStep;
  const Int  iStrideOrg      = rcDtParam.org.stride * iSubStep;
  const UInt distortionShift = DISTORTION_PRECISION_ADJUSTMENT(rcDtParam.bitDepth - 8);

  Distortion uiSum = 0;

  for( ; iRows != 0; iRows -= iSubStep )
  {
    for (Int n = 0; n < iCols; n++ )
    {
      uiSum += abs( piOrg[n] - piCur[n] );
    }
    if (rcDtParam.maximumDistortionForEarlyExit < ( uiSum >> distortionShift ))
    {
      return ( uiSum >> distortionShift );
    }
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  uiSum <<= iSubShift;
  return ( uiSum >> distortionShift );
}

Distortion MyDistortionCost::xGetSAD4( const DistParam& rcDtParam )
{
  if ( rcDtParam.applyWeight )
  {
    return RdCostWeightPrediction::xGetSADw( rcDtParam );
  }

  const Pel* piOrg   = rcDtParam.org.buf;
  const Pel* piCur   = rcDtParam.cur.buf;
  Int  iRows         = rcDtParam.org.height;
  Int  iSubShift     = rcDtParam.subShift;
  Int  iSubStep      = ( 1 << iSubShift );
  Int  iStrideCur    = rcDtParam.cur.stride * iSubStep;
  Int  iStrideOrg    = rcDtParam.org.stride * iSubStep;

  Distortion uiSum = 0;

  for( ; iRows != 0; iRows -= iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  uiSum <<= iSubShift;
  return ( uiSum >> DISTORTION_PRECISION_ADJUSTMENT(rcDtParam.bitDepth-8) );
}

Distortion MyDistortionCost::xGetSAD8( const DistParam& rcDtParam )
{
  if ( rcDtParam.applyWeight )
  {
    return RdCostWeightPrediction::xGetSADw( rcDtParam );
  }

  const Pel* piOrg      = rcDtParam.org.buf;
  const Pel* piCur      = rcDtParam.cur.buf;
  Int  iRows            = rcDtParam.org.height;
  Int  iSubShift        = rcDtParam.subShift;
  Int  iSubStep         = ( 1 << iSubShift );
  Int  iStrideCur       = rcDtParam.cur.stride * iSubStep;
  Int  iStrideOrg       = rcDtParam.org.stride * iSubStep;

  Distortion uiSum = 0;

  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  uiSum <<= iSubShift;
  return ( uiSum >> DISTORTION_PRECISION_ADJUSTMENT(rcDtParam.bitDepth-8) );
}

Distortion MyDistortionCost::xGetSAD16( const DistParam& rcDtParam )
{
  if ( rcDtParam.applyWeight )
  {
    return RdCostWeightPrediction::xGetSADw( rcDtParam );
  }

  const Pel* piOrg      = rcDtParam.org.buf;
  const Pel* piCur      = rcDtParam.cur.buf;
  Int  iRows            = rcDtParam.org.height;
  Int  iSubShift        = rcDtParam.subShift;
  Int  iSubStep         = ( 1 << iSubShift );
  Int  iStrideCur       = rcDtParam.cur.stride * iSubStep;
  Int  iStrideOrg       = rcDtParam.org.stride * iSubStep;

  Distortion uiSum = 0;

  for( ; iRows != 0; iRows -= iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );
    uiSum += abs( piOrg[12] - piCur[12] );
    uiSum += abs( piOrg[13] - piCur[13] );
    uiSum += abs( piOrg[14] - piCur[14] );
    uiSum += abs( piOrg[15] - piCur[15] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  uiSum <<= iSubShift;
  return ( uiSum >> DISTORTION_PRECISION_ADJUSTMENT(rcDtParam.bitDepth-8) );
}

Distortion MyDistortionCost::xGetSAD12( const DistParam& rcDtParam )
{
  if ( rcDtParam.applyWeight )
  {
    return RdCostWeightPrediction::xGetSADw( rcDtParam );
  }

  const Pel* piOrg      = rcDtParam.org.buf;
  const Pel* piCur      = rcDtParam.cur.buf;
  Int  iRows            = rcDtParam.org.height;
  Int  iSubShift        = rcDtParam.subShift;
  Int  iSubStep         = ( 1 << iSubShift );
  Int  iStrideCur       = rcDtParam.cur.stride * iSubStep;
  Int  iStrideOrg       = rcDtParam.org.stride * iSubStep;

  Distortion uiSum = 0;

  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  uiSum <<= iSubShift;
  return ( uiSum >> DISTORTION_PRECISION_ADJUSTMENT(rcDtParam.bitDepth-8) );
}

Distortion MyDistortionCost::xGetSAD16N( const DistParam &rcDtParam )
{
  const Pel* piOrg  = rcDtParam.org.buf;
  const Pel* piCur  = rcDtParam.cur.buf;
  Int  iRows        = rcDtParam.org.height;
  Int  iCols        = rcDtParam.org.width;
  Int  iSubShift    = rcDtParam.subShift;
  Int  iSubStep     = ( 1 << iSubShift );
  Int  iStrideCur   = rcDtParam.cur.stride * iSubStep;
  Int  iStrideOrg   = rcDtParam.org.stride * iSubStep;

  Distortion uiSum = 0;

  for( ; iRows != 0; iRows-=iSubStep )
  {
    for (Int n = 0; n < iCols; n+=16 )
    {
      uiSum += abs( piOrg[n+ 0] - piCur[n+ 0] );
      uiSum += abs( piOrg[n+ 1] - piCur[n+ 1] );
      uiSum += abs( piOrg[n+ 2] - piCur[n+ 2] );
      uiSum += abs( piOrg[n+ 3] - piCur[n+ 3] );
      uiSum += abs( piOrg[n+ 4] - piCur[n+ 4] );
      uiSum += abs( piOrg[n+ 5] - piCur[n+ 5] );
      uiSum += abs( piOrg[n+ 6] - piCur[n+ 6] );
      uiSum += abs( piOrg[n+ 7] - piCur[n+ 7] );
      uiSum += abs( piOrg[n+ 8] - piCur[n+ 8] );
      uiSum += abs( piOrg[n+ 9] - piCur[n+ 9] );
      uiSum += abs( piOrg[n+10] - piCur[n+10] );
      uiSum += abs( piOrg[n+11] - piCur[n+11] );
      uiSum += abs( piOrg[n+12] - piCur[n+12] );
      uiSum += abs( piOrg[n+13] - piCur[n+13] );
      uiSum += abs( piOrg[n+14] - piCur[n+14] );
      uiSum += abs( piOrg[n+15] - piCur[n+15] );
    }
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  uiSum <<= iSubShift;
  return ( uiSum >> DISTORTION_PRECISION_ADJUSTMENT(rcDtParam.bitDepth-8) );
}

Distortion MyDistortionCost::xGetSAD32( const DistParam &rcDtParam )
{
  if ( rcDtParam.applyWeight )
  {
    return RdCostWeightPrediction::xGetSADw( rcDtParam );
  }

  const Pel* piOrg      = rcDtParam.org.buf;
  const Pel* piCur      = rcDtParam.cur.buf;
  Int  iRows            = rcDtParam.org.height;
  Int  iSubShift        = rcDtParam.subShift;
  Int  iSubStep         = ( 1 << iSubShift );
  Int  iStrideCur       = rcDtParam.cur.stride * iSubStep;
  Int  iStrideOrg       = rcDtParam.org.stride * iSubStep;

  Distortion uiSum = 0;

  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );
    uiSum += abs( piOrg[12] - piCur[12] );
    uiSum += abs( piOrg[13] - piCur[13] );
    uiSum += abs( piOrg[14] - piCur[14] );
    uiSum += abs( piOrg[15] - piCur[15] );
    uiSum += abs( piOrg[16] - piCur[16] );
    uiSum += abs( piOrg[17] - piCur[17] );
    uiSum += abs( piOrg[18] - piCur[18] );
    uiSum += abs( piOrg[19] - piCur[19] );
    uiSum += abs( piOrg[20] - piCur[20] );
    uiSum += abs( piOrg[21] - piCur[21] );
    uiSum += abs( piOrg[22] - piCur[22] );
    uiSum += abs( piOrg[23] - piCur[23] );
    uiSum += abs( piOrg[24] - piCur[24] );
    uiSum += abs( piOrg[25] - piCur[25] );
    uiSum += abs( piOrg[26] - piCur[26] );
    uiSum += abs( piOrg[27] - piCur[27] );
    uiSum += abs( piOrg[28] - piCur[28] );
    uiSum += abs( piOrg[29] - piCur[29] );
    uiSum += abs( piOrg[30] - piCur[30] );
    uiSum += abs( piOrg[31] - piCur[31] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  uiSum <<= iSubShift;
  return ( uiSum >> DISTORTION_PRECISION_ADJUSTMENT(rcDtParam.bitDepth-8) );
}

Distortion MyDistortionCost::xGetSAD24( const DistParam &rcDtParam )
{
  if ( rcDtParam.applyWeight )
  {
    return RdCostWeightPrediction::xGetSADw( rcDtParam );
  }

  const Pel* piOrg      = rcDtParam.org.buf;
  const Pel* piCur      = rcDtParam.cur.buf;
  Int  iRows            = rcDtParam.org.height;
  Int  iSubShift        = rcDtParam.subShift;
  Int  iSubStep         = ( 1 << iSubShift );
  Int  iStrideCur       = rcDtParam.cur.stride * iSubStep;
  Int  iStrideOrg       = rcDtParam.org.stride * iSubStep;

  Distortion uiSum = 0;

  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );
    uiSum += abs( piOrg[12] - piCur[12] );
    uiSum += abs( piOrg[13] - piCur[13] );
    uiSum += abs( piOrg[14] - piCur[14] );
    uiSum += abs( piOrg[15] - piCur[15] );
    uiSum += abs( piOrg[16] - piCur[16] );
    uiSum += abs( piOrg[17] - piCur[17] );
    uiSum += abs( piOrg[18] - piCur[18] );
    uiSum += abs( piOrg[19] - piCur[19] );
    uiSum += abs( piOrg[20] - piCur[20] );
    uiSum += abs( piOrg[21] - piCur[21] );
    uiSum += abs( piOrg[22] - piCur[22] );
    uiSum += abs( piOrg[23] - piCur[23] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  uiSum <<= iSubShift;
  return ( uiSum >> DISTORTION_PRECISION_ADJUSTMENT(rcDtParam.bitDepth-8) );
}

Distortion MyDistortionCost::xGetSAD64( const DistParam &rcDtParam )
{
  if ( rcDtParam.applyWeight )
  {
    return RdCostWeightPrediction::xGetSADw( rcDtParam );
  }

  const Pel* piOrg      = rcDtParam.org.buf;
  const Pel* piCur      = rcDtParam.cur.buf;
  Int  iRows            = rcDtParam.org.height;
  Int  iSubShift        = rcDtParam.subShift;
  Int  iSubStep         = ( 1 << iSubShift );
  Int  iStrideCur       = rcDtParam.cur.stride * iSubStep;
  Int  iStrideOrg       = rcDtParam.org.stride * iSubStep;

  Distortion uiSum = 0;

  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );
    uiSum += abs( piOrg[12] - piCur[12] );
    uiSum += abs( piOrg[13] - piCur[13] );
    uiSum += abs( piOrg[14] - piCur[14] );
    uiSum += abs( piOrg[15] - piCur[15] );
    uiSum += abs( piOrg[16] - piCur[16] );
    uiSum += abs( piOrg[17] - piCur[17] );
    uiSum += abs( piOrg[18] - piCur[18] );
    uiSum += abs( piOrg[19] - piCur[19] );
    uiSum += abs( piOrg[20] - piCur[20] );
    uiSum += abs( piOrg[21] - piCur[21] );
    uiSum += abs( piOrg[22] - piCur[22] );
    uiSum += abs( piOrg[23] - piCur[23] );
    uiSum += abs( piOrg[24] - piCur[24] );
    uiSum += abs( piOrg[25] - piCur[25] );
    uiSum += abs( piOrg[26] - piCur[26] );
    uiSum += abs( piOrg[27] - piCur[27] );
    uiSum += abs( piOrg[28] - piCur[28] );
    uiSum += abs( piOrg[29] - piCur[29] );
    uiSum += abs( piOrg[30] - piCur[30] );
    uiSum += abs( piOrg[31] - piCur[31] );
    uiSum += abs( piOrg[32] - piCur[32] );
    uiSum += abs( piOrg[33] - piCur[33] );
    uiSum += abs( piOrg[34] - piCur[34] );
    uiSum += abs( piOrg[35] - piCur[35] );
    uiSum += abs( piOrg[36] - piCur[36] );
    uiSum += abs( piOrg[37] - piCur[37] );
    uiSum += abs( piOrg[38] - piCur[38] );
    uiSum += abs( piOrg[39] - piCur[39] );
    uiSum += abs( piOrg[40] - piCur[40] );
    uiSum += abs( piOrg[41] - piCur[41] );
    uiSum += abs( piOrg[42] - piCur[42] );
    uiSum += abs( piOrg[43] - piCur[43] );
    uiSum += abs( piOrg[44] - piCur[44] );
    uiSum += abs( piOrg[45] - piCur[45] );
    uiSum += abs( piOrg[46] - piCur[46] );
    uiSum += abs( piOrg[47] - piCur[47] );
    uiSum += abs( piOrg[48] - piCur[48] );
    uiSum += abs( piOrg[49] - piCur[49] );
    uiSum += abs( piOrg[50] - piCur[50] );
    uiSum += abs( piOrg[51] - piCur[51] );
    uiSum += abs( piOrg[52] - piCur[52] );
    uiSum += abs( piOrg[53] - piCur[53] );
    uiSum += abs( piOrg[54] - piCur[54] );
    uiSum += abs( piOrg[55] - piCur[55] );
    uiSum += abs( piOrg[56] - piCur[56] );
    uiSum += abs( piOrg[57] - piCur[57] );
    uiSum += abs( piOrg[58] - piCur[58] );
    uiSum += abs( piOrg[59] - piCur[59] );
    uiSum += abs( piOrg[60] - piCur[60] );
    uiSum += abs( piOrg[61] - piCur[61] );
    uiSum += abs( piOrg[62] - piCur[62] );
    uiSum += abs( piOrg[63] - piCur[63] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  uiSum <<= iSubShift;
  return ( uiSum >> DISTORTION_PRECISION_ADJUSTMENT(rcDtParam.bitDepth-8) );
}

Distortion MyDistortionCost::xGetSAD48( const DistParam &rcDtParam )
{
  if ( rcDtParam.applyWeight )
  {
    return RdCostWeightPrediction::xGetSADw( rcDtParam );
  }

  const Pel* piOrg      = rcDtParam.org.buf;
  const Pel* piCur      = rcDtParam.cur.buf;
  Int  iRows            = rcDtParam.org.height;
  Int  iSubShift        = rcDtParam.subShift;
  Int  iSubStep         = ( 1 << iSubShift );
  Int  iStrideCur       = rcDtParam.cur.stride * iSubStep;
  Int  iStrideOrg       = rcDtParam.org.stride * iSubStep;

  Distortion uiSum = 0;

  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );
    uiSum += abs( piOrg[12] - piCur[12] );
    uiSum += abs( piOrg[13] - piCur[13] );
    uiSum += abs( piOrg[14] - piCur[14] );
    uiSum += abs( piOrg[15] - piCur[15] );
    uiSum += abs( piOrg[16] - piCur[16] );
    uiSum += abs( piOrg[17] - piCur[17] );
    uiSum += abs( piOrg[18] - piCur[18] );
    uiSum += abs( piOrg[19] - piCur[19] );
    uiSum += abs( piOrg[20] - piCur[20] );
    uiSum += abs( piOrg[21] - piCur[21] );
    uiSum += abs( piOrg[22] - piCur[22] );
    uiSum += abs( piOrg[23] - piCur[23] );
    uiSum += abs( piOrg[24] - piCur[24] );
    uiSum += abs( piOrg[25] - piCur[25] );
    uiSum += abs( piOrg[26] - piCur[26] );
    uiSum += abs( piOrg[27] - piCur[27] );
    uiSum += abs( piOrg[28] - piCur[28] );
    uiSum += abs( piOrg[29] - piCur[29] );
    uiSum += abs( piOrg[30] - piCur[30] );
    uiSum += abs( piOrg[31] - piCur[31] );
    uiSum += abs( piOrg[32] - piCur[32] );
    uiSum += abs( piOrg[33] - piCur[33] );
    uiSum += abs( piOrg[34] - piCur[34] );
    uiSum += abs( piOrg[35] - piCur[35] );
    uiSum += abs( piOrg[36] - piCur[36] );
    uiSum += abs( piOrg[37] - piCur[37] );
    uiSum += abs( piOrg[38] - piCur[38] );
    uiSum += abs( piOrg[39] - piCur[39] );
    uiSum += abs( piOrg[40] - piCur[40] );
    uiSum += abs( piOrg[41] - piCur[41] );
    uiSum += abs( piOrg[42] - piCur[42] );
    uiSum += abs( piOrg[43] - piCur[43] );
    uiSum += abs( piOrg[44] - piCur[44] );
    uiSum += abs( piOrg[45] - piCur[45] );
    uiSum += abs( piOrg[46] - piCur[46] );
    uiSum += abs( piOrg[47] - piCur[47] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  uiSum <<= iSubShift;
  return ( uiSum >> DISTORTION_PRECISION_ADJUSTMENT(rcDtParam.bitDepth-8) );
}
