/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/
#ifndef MYQUANT_H
#define MYQUANT_H
#include "CommonLib/CommonDef.h"
#include "CommonLib/Unit.h"
#include "video_common.h"
#include <CommonLib/Buffer.h>
#include "encodingstructure.h"
#include "mytransform2d.h"
#include <map>

/// QP struct
struct MyQpParam
{
  Int Qp;
  Int per;
  Int rem;

  ChannelType channelType;
  ChromaFormat chromaFomat;

  MyQpParam(const Int           qpy,
            const ChannelType   chType,
            const ChromaFormat  chFmt,
            const Int channelBitDepth,
            const Int           chromaQPOffset=0,
            const int           dqp=0 );

}; // END STRUCT DEFINITION QpParam

//struct Key_QuantizationMatrix : public Size
//{
//    Key_QuantizationMatrix(const SizeType Width, const SizeType Height, const Int qp) : Size(Width, Height), qp_rem(qp) {}
//    Bool operator==(const Key_QuantizationMatrix &other) const { return (Size::operator==(other)) && (qp_rem==other.qp_rem); }
//    Bool operator==(const Key_QuantizationMatrix &other) const { return !(*this==other);}
    
//    Int qp_rem;
//};

class MyQuant
{
protected:
    struct WeightingMatrixComparator
    {

        bool operator()(const std::pair<Size, Int> & left, const std::pair<Size, Int> & right) const
        {
            return      left.first.width < right.first.width
                     || ( !(right.first.width < left.first.width) && left.first.height < right.first.height )
                     || ( !(right.first.width < left.first.width) && !(right.first.height < left.first.height) && left.second < right.second);
        }
    };

    std::map<std::pair<Size, Int>, AreaBuf<Int>,  WeightingMatrixComparator> _mapQuantizerWeightingMatrix; // quantization matrix for a specific size (UInt width, UInt height) and qp_rem (remainder (q)
    std::map<std::pair<Size, Int>, AreaBuf<Int>,  WeightingMatrixComparator> _mapDequantizerWeightingMatrix; // quantization matrix for a specific size (UInt width, UInt height) and qp_rem (remainder (q)
public:
    MyQuant();
    ~MyQuant();
    const AreaBuf<const Int> getDefaultQuantizationMatrix(const Size& size,  const MyQpParam &cQP, const SliceType sliceType);
    const AreaBuf<const Int> getDefaultDequantizationMatrix(const Size& size,  const MyQpParam &cQP, const SliceType sliceType);
    // quantization
    virtual Void quant(CoeffBuf& piQCoef, const CCoeffBuf &pSrc, TCoeff &uiAbsSum, const MyQpParam &cQP,  const SliceType sliceType, const Int channelBitDepth, const int maxLog2TrDynamicRange =15);
    virtual Void quant(CoeffBuf& piQCoef, const CCoeffBuf &pSrc, const AreaBuf<const Int> &qWeightingMatrix, TCoeff &uiAbsSum, const MyQpParam &cQP,  const SliceType sliceType, const Int channelBitDepth, const int maxLog2TrDynamicRange =15);
    virtual Void dequant(CoeffBuf& piCoef, const CCoeffBuf &piQCoef, const MyQpParam &cQP, const Int channelBitDepth, const int maxLog2TrDynamicRange =15);

    static void quantizeFrame(const int qp, const PelUnitBuf &originalImage, PelUnitBuf &quantizedImage, EncodingStructure &encStruct, const ComponentID compID, const int bitDepths[MAX_NUM_CHANNEL_TYPE], int maxLog2TrDynamicRange);

    static void quantizeFrame(const int qp, const PelUnitBuf &originalImage, PelUnitBuf &quantizedImage, EncodingStructure &encStruct, const int bitDepths[MAX_NUM_CHANNEL_TYPE], int maxLog2TrDynamicRange);


};

#endif // MYQUANT_H
