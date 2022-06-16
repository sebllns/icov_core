/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/
#pragma once

#include "video_common.h"
#include "QpParam.h"
#include "CommonLib/Unit.h"
#include <map>

namespace icov {
namespace video {

class ICOV_Quant
{
protected:
    struct WeightingMatrixComparator
    {

        bool operator()(const std::pair<Size, int> & left, const std::pair<Size, int> & right) const
        {
            return      left.first.width < right.first.width
                    || ( !(right.first.width < left.first.width) && left.first.height < right.first.height )
                    || ( !(right.first.width < left.first.width) && !(right.first.height < left.first.height) && left.second < right.second);
        }
    };

    std::map<std::pair<Size, int>, AreaBuf<int>,  WeightingMatrixComparator> _mapQuantizerWeightingMatrix; // quantization matrix for a specific size (Uint width, Uint height) and qp_rem (remainder (q)
    std::map<std::pair<Size, int>, AreaBuf<int>,  WeightingMatrixComparator> _mapDequantizerWeightingMatrix; // quantization matrix for a specific size (Uint width, Uint height) and qp_rem (remainder (q)
public:
    ICOV_Quant();
    ~ICOV_Quant();
    const AreaBuf<const int> getDefaultQuantizationMatrix(const Size& size,  const icov::video::ICOV_QpParam &cQP, const SliceType sliceType);
    const AreaBuf<const int> getDefaultDequantizationMatrix(const Size& size,  const icov::video::ICOV_QpParam &cQP, const SliceType sliceType);
    // quantization
    virtual Void quant(CoeffBuf& piQCoef, const CCoeffBuf &pSrc, TCoeff &uiAbsSum, const icov::video::ICOV_QpParam &cQP,  const SliceType sliceType, const int channelBitDepth, const int maxLog2TrDynamicRange =15);
    virtual Void quant(CoeffBuf& piQCoef, const CCoeffBuf &pSrc, const AreaBuf<const int> &qWeightingMatrix, TCoeff &uiAbsSum, const icov::video::ICOV_QpParam &cQP,  const SliceType sliceType, const int channelBitDepth, const int maxLog2TrDynamicRange =15);
    virtual Void dequant(CoeffBuf& piCoef, const CCoeffBuf &piQCoef, const icov::video::ICOV_QpParam &cQP, const int channelBitDepth, const int maxLog2TrDynamicRange =15);
};
}
}
