/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/
#include "myquant.h"
#include "CommonLib/UnitTools.h"

MyQpParam::MyQpParam(const Int qpy, const ChannelType chType, const ChromaFormat chFmt, const Int channelBitDepth, const Int chromaQPOffset, const int dqp)
{

    channelType = chType;
    chromaFomat = chFmt;
    /*
        the quantizer step size is in the range of 0.630 ≤ Δq ≤ 228.1. Thereby, the maximum
        quantizer step size is in the range of the maximum amplitude of the 8 bit input signal.
        For higher bit-depths, a QP offset is specified, increasing the QP parameter range by
        6 per additional bit of signal bit depth towards finer quantization (i.e. towards lower
        QP values).
     */
    const Int qpBdOffset = 6 * (channelBitDepth - 8);
    Int baseQp;

    if(isLuma(chType))
    {
        baseQp = qpy + qpBdOffset;
    }
    else
    {
        baseQp = Clip3( -qpBdOffset, (chromaQPMappingTableSize - 1), qpy + chromaQPOffset );

        if(baseQp < 0)
        {
            baseQp = baseQp + qpBdOffset;
        }
        else
        {
            baseQp = getScaledChromaQP(baseQp, chFmt) + qpBdOffset;
        }
    }

    baseQp = Clip3( 0, MAX_QP+qpBdOffset, baseQp + dqp );

    Qp =baseQp;
    per=baseQp/6;
    rem=baseQp%6;

}



MyQuant::MyQuant()
{

}

MyQuant::~MyQuant()
{

    for (auto& kv : _mapQuantizerWeightingMatrix)
    {
        if(kv.second.buf)
        {
            xFree(kv.second.buf);
            kv.second.buf = nullptr;
        }
    }
}

const Int quantTSDefault4x4[4*4] =
{
  16,16,16,16,
  16,16,16,16,
  16,16,16,16,
  16,16,16,16
};

const Int quantIntraDefault8x8[8*8] =
{
  16,16,16,16,17,18,21,24,
  16,16,16,16,17,19,22,25,
  16,16,17,18,20,22,25,29,
  16,16,18,21,24,27,31,36,
  17,17,20,24,30,35,41,47,
  18,19,22,27,35,44,54,65,
  21,22,25,31,41,54,70,88,
  24,25,29,36,47,65,88,115
};

const Int quantInterDefault8x8[8*8] =
{
  16,16,16,16,17,18,20,24,
  16,16,16,17,18,20,24,25,
  16,16,17,18,20,24,25,28,
  16,17,18,20,24,25,28,33,
  17,18,20,24,25,28,33,41,
  18,20,24,25,28,33,41,54,
  20,24,25,28,33,41,54,71,
  24,25,28,33,41,54,71,91
};

const AreaBuf<const Int> MyQuant::getDefaultQuantizationMatrix(const Size& size,  const MyQpParam &cQP, const SliceType sliceType)
{
//    std::map<std::pair<Size, Int>, AreaBuf<Int>,  WeightingMatrixComparator> _mapQuantizerWeightingMatrix; // quantization matrix for a specific size (UInt width, UInt height) and qp_rem (remainder (q)

    std::pair<Size, Int> key(size, cQP.rem);
    auto it = _mapQuantizerWeightingMatrix.find(key);
    if (it != _mapQuantizerWeightingMatrix.end())
        return (it->second);

    // if it does not exist
    const Int* weights = nullptr;
    UInt max_length = std::max(size.width, size.height);
    UInt sizeQuantMatrixBase = 0;

    if(max_length <= 4)
    {
        weights = quantTSDefault4x4;
        sizeQuantMatrixBase = 4;
    }
    else if(sliceType==I_SLICE)
    {
        weights =  quantIntraDefault8x8;
        sizeQuantMatrixBase = 8;
    }
    else
    {
        weights =  quantInterDefault8x8;
        sizeQuantMatrixBase = 8;
    }

    int* dstBuf = (int*) xMalloc (int, size.area());
    AreaBuf<Int> areaBuf(dstBuf, size);

    Int quantScales = g_quantScales[cQP.rem];
    quantScales = quantScales << LOG2_SCALING_LIST_NEUTRAL_VALUE;
    UInt ratio_x, ratio_y;  // it is on purpose that we used UInt, because later we need only integer part of division
    ratio_x = size.width  / sizeQuantMatrixBase;
    if(ratio_x==0)
        ratio_x = 1;
    ratio_y = size.height / sizeQuantMatrixBase;
    if(ratio_y==0)
        ratio_y = 1;


//    std::cout << "weight coeffs=[ \n";
//    for(UInt j=0;j<size.height;j++)
//    {
//        for(UInt i=0;i<size.width;i++)
//        {
//            std::cout << weights[sizeQuantMatrixBase * (j / ratio_y) + i / ratio_x] << ",\t";
//        }
//        std::cout << std::endl;
//    }
//    std::cout << "]" << std::endl;
    for(UInt y=0;y<size.height;y++)
    {
        Int* areaBuf_rowPtr = &areaBuf.at(0,y);
        for(UInt x=0;x<size.width;x++)
        {
            areaBuf_rowPtr[x] = quantScales / weights[sizeQuantMatrixBase * (y / ratio_y) + x / ratio_x];
        }
    }

    _mapQuantizerWeightingMatrix[key] = areaBuf;

    return areaBuf;

}

const AreaBuf<const Int> MyQuant::getDefaultDequantizationMatrix(const Size &size, const MyQpParam &cQP, const SliceType sliceType)
{

}

Void MyQuant::quant(CoeffBuf &piQCoef, const CCoeffBuf &pSrc, TCoeff &uiAbsSum, const MyQpParam &cQP, const SliceType sliceType, const Int channelBitDepth, const int maxLog2TrDynamicRange)
{

    CHECK( piQCoef.width!= pSrc.width || piQCoef.height!= pSrc.height, "Sizes do not match");
    const TCoeff entropyCodingMinimum = -(1 << maxLog2TrDynamicRange);
    const TCoeff entropyCodingMaximum =  (1 << maxLog2TrDynamicRange) - 1;

    const Int  defaultQuantisationCoefficient = g_quantScales[cQP.rem];
    const Size& size = pSrc;

    /* for 422 chroma blocks, the effective scaling applied during transformation is not a power of 2, hence it cannot be
     * implemented as a bit-shift (the quantised result will be sqrt(2) * larger than required). Alternatively, adjust the
     * uiLog2TrSize applied in iTransformShift, such that the result is 1/sqrt(2) the required result (i.e. smaller)
     * Then a QP+3 (sqrt(2)) or QP-3 (1/sqrt(2)) method could be used to get the required result
     */
    // Represents scaling through forward transform
    Int iTransformShift = getTransformShift(channelBitDepth, size, maxLog2TrDynamicRange);

    Int iWHScale = 1;
    if( TU::needsBlockSizeTrafoScale( size ) )
    {
      iTransformShift += ADJ_QUANT_SHIFT;
      iWHScale = 181;
    }

    const Int iQBits = QUANT_SHIFT + cQP.per + iTransformShift;
    // QBits will be OK for any internal bit depth as the reduction in transform shift is balanced by an increase in Qp_per due to QpBDOffset

    const Int64 iAdd = Int64(sliceType == I_SLICE ? 171 : 85) << Int64(iQBits - 9);

    UInt uiHeight = size.height;
    UInt uiWidth  = size.width;
    if ( (int(pSrc.width)==pSrc.stride) && (int(piQCoef.width)==piQCoef.stride))  // if both are continous. it is silly that in HEVC stride is int instead of UInt
    {
        uiWidth *= uiHeight;
        uiHeight = 1;
    }
    for(UInt y=0; y < uiHeight; y++)  // TODO if we had something like continous, then we could easily to the trick if(pSrc.isCountinous && piQCoef.isContinous) then : iWdith =iHeight*iWdith, iHeight=1 (similar trick that we use in OpenCV)
    {
        const TCoeff* pSrc_rowPtr = &pSrc.at(0,y);
        TCoeff* piQCoef_rowPtr = &piQCoef.at(0,y);
        for(UInt x=0; x < uiWidth; x++)
        {
            const TCoeff& iLevel= pSrc_rowPtr[x];
            const TCoeff iSign    = (iLevel < 0 ? -1: 1);

            const Int64  tmpLevel = (Int64)abs(iLevel) * defaultQuantisationCoefficient;

            const TCoeff quantisedMagnitude = TCoeff((tmpLevel * iWHScale + iAdd ) >> iQBits);

            uiAbsSum += quantisedMagnitude;
            const TCoeff quantisedCoefficient = quantisedMagnitude * iSign;

            piQCoef_rowPtr[x] = Clip3<TCoeff>( entropyCodingMinimum, entropyCodingMaximum, quantisedCoefficient );
        }
    }
}

Void MyQuant::quant(CoeffBuf &piQCoef, const CCoeffBuf &pSrc, const AreaBuf<const Int> &qWeightingMatrix, TCoeff &uiAbsSum, const MyQpParam &cQP, const SliceType sliceType, const Int channelBitDepth, const int maxLog2TrDynamicRange)
{
    CHECK( piQCoef.width!= pSrc.width || piQCoef.height!= pSrc.height, "Sizes do not match");
    CHECK( qWeightingMatrix.width!= pSrc.width || qWeightingMatrix.height!= pSrc.height, "Sizes do not match");
    const TCoeff entropyCodingMinimum = -(1 << maxLog2TrDynamicRange);
    const TCoeff entropyCodingMaximum =  (1 << maxLog2TrDynamicRange) - 1;

    const Size& size = pSrc;

    /* for 422 chroma blocks, the effective scaling applied during transformation is not a power of 2, hence it cannot be
     * implemented as a bit-shift (the quantised result will be sqrt(2) * larger than required). Alternatively, adjust the
     * uiLog2TrSize applied in iTransformShift, such that the result is 1/sqrt(2) the required result (i.e. smaller)
     * Then a QP+3 (sqrt(2)) or QP-3 (1/sqrt(2)) method could be used to get the required result
     */
    // Represents scaling through forward transform
    Int iTransformShift = getTransformShift(channelBitDepth, size, maxLog2TrDynamicRange);

    Int iWHScale = 1;
    if( TU::needsBlockSizeTrafoScale( size ) )
    {
      iTransformShift += ADJ_QUANT_SHIFT;
      iWHScale = 181;
    }

    const Int iQBits = QUANT_SHIFT + cQP.per + iTransformShift;
    // QBits will be OK for any internal bit depth as the reduction in transform shift is balanced by an increase in Qp_per due to QpBDOffset

    const Int64 iAdd = Int64(sliceType == I_SLICE ? 171 : 85) << Int64(iQBits - 9);

    UInt uiHeight = size.height;
    UInt uiWidth  = size.width;
    if ( (int(pSrc.width)==pSrc.stride) && (int(piQCoef.width)==piQCoef.stride) && (int(qWeightingMatrix.width)==qWeightingMatrix.stride))  // if both are continous. it is silly that in HEVC stride is int instead of UInt
    {
        uiWidth *= uiHeight;
        uiHeight = 1;
    }
    for(UInt y=0; y < uiHeight; y++)  // TODO if we had something like continous, then we could easily to the trick if(pSrc.isCountinous && piQCoef.isContinous) then : iWdith =iHeight*iWdith, iHeight=1 (similar trick that we use in OpenCV)
    {
        const TCoeff* pSrc_rowPtr = &pSrc.at(0,y);
        const Int* qWeightingMatrix_rowPtr = &qWeightingMatrix.at(0,y); // Int and TCoeff are both int, but just to emphesize it is int, I used int here
        TCoeff* piQCoef_rowPtr = &piQCoef.at(0,y);
        for(UInt x=0; x < uiWidth; x++)
        {
            const TCoeff& iLevel= pSrc_rowPtr[x];
            const TCoeff iSign    = (iLevel < 0 ? -1: 1);

            const Int64  tmpLevel = (Int64)abs(iLevel) * qWeightingMatrix_rowPtr[x];

            const TCoeff quantisedMagnitude = TCoeff((tmpLevel * iWHScale + iAdd ) >> iQBits);

            uiAbsSum += quantisedMagnitude;
            const TCoeff quantisedCoefficient = quantisedMagnitude * iSign;

            piQCoef_rowPtr[x] = Clip3<TCoeff>( entropyCodingMinimum, entropyCodingMaximum, quantisedCoefficient );
        }
    }
}

Void MyQuant::dequant(CoeffBuf &piCoef, const CCoeffBuf &piQCoef, const MyQpParam &cQP, const Int channelBitDepth, const int maxLog2TrDynamicRange)
{

    CHECK( piQCoef.width!= piCoef.width || piQCoef.height!= piCoef.height, "Sizes do not match");
    const TCoeff          transformMinimum   = -(1 << maxLog2TrDynamicRange);
    const TCoeff          transformMaximum   =  (1 << maxLog2TrDynamicRange) - 1;
    const Size& size = piQCoef;

    const Int  iTransformShift = getTransformShift(channelBitDepth, size, maxLog2TrDynamicRange);

    const Int QP_per = cQP.per;
    const Int QP_rem = cQP.rem;

    const Bool needsScalingCorrection = TU::needsBlockSizeTrafoScale( size );
    const Int  NEScale    = TU::needsSqrt2Scale( size ) ? 181 : 1;

    const Int  rightShift = (needsScalingCorrection ?   8 : 0 ) + (IQUANT_SHIFT - (iTransformShift + QP_per));
    const Int scale     = g_invQuantScales[QP_rem] * NEScale;

    const Int scaleBits = ( IQUANT_SHIFT + 1 );

    const UInt             targetInputBitDepth = std::min<UInt>((maxLog2TrDynamicRange + 1), (((sizeof(Intermediate_Int) * 8) + rightShift) - scaleBits));
    const Intermediate_Int inputMinimum        = -(1 << (targetInputBitDepth - 1));
    const Intermediate_Int inputMaximum        =  (1 << (targetInputBitDepth - 1)) - 1;

    UInt uiHeight = size.height;
    UInt uiWidth  = size.width;
    if (rightShift > 0)
    {
      const Intermediate_Int iAdd = 1 << (rightShift - 1);

      for(UInt y=0; y < uiHeight; y++)    // TODO if we had something like continous, then we could easily to the trick if(pSrc.isCountinous && piQCoef.isContinous) then : iWdith =iHeight*iWdith, iHeight=1 (similar trick that we use in OpenCV)
      {
          const TCoeff* piQCoef_rowPtr = &piQCoef.at(0,y);
          TCoeff* piCoef_rowPtr = &piCoef.at(0,y);

          for(UInt x=0; x < uiWidth; x++)
          {
              const TCoeff           clipQCoef = TCoeff(Clip3<Intermediate_Int>(inputMinimum, inputMaximum, piQCoef_rowPtr[x]));
              const Intermediate_Int iCoeffQ   = (Intermediate_Int(clipQCoef) * scale + iAdd) >> rightShift;

              piCoef_rowPtr[x] = TCoeff(Clip3<Intermediate_Int>(transformMinimum,transformMaximum,iCoeffQ));
          }
      }
    }
    else
    {
      const Int leftShift = -rightShift;

      for(UInt y=0; y < uiHeight; y++)    // TODO if we had something like continous, then we could easily to the trick if(pSrc.isCountinous && piQCoef.isContinous) then : iWdith =iHeight*iWdith, iHeight=1 (similar trick that we use in OpenCV)
      {
          const TCoeff* piQCoef_rowPtr = &piQCoef.at(0,y);
          TCoeff* piCoef_rowPtr = &piCoef.at(0,y);

          for(UInt x=0; x < uiWidth; x++)
          {
              const TCoeff           clipQCoef = TCoeff(Clip3<Intermediate_Int>(inputMinimum, inputMaximum, piQCoef_rowPtr[x]));
              const Intermediate_Int iCoeffQ   = (Intermediate_Int(clipQCoef) * scale) << leftShift;

              piCoef_rowPtr[x] = TCoeff(Clip3<Intermediate_Int>(transformMinimum,transformMaximum,iCoeffQ));
          }
      }
    }
}

