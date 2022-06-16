#include "quantizeFrame.h"

void quantizeFrame(const int qp, const PelUnitBuf &originalImage, PelUnitBuf &quantizedImage, EncodingStructure &encStruct, const int bitDepths[], int maxLog2TrDynamicRange)
{
    if(originalImage.chromaFormat != quantizedImage.chromaFormat)
        throw std::runtime_error("The original image and the quantized image should have similar chroma");

    const UInt numCh = getNumberValidComponents( originalImage.chromaFormat );

    for( UInt ch = 0; ch < numCh; ch++ )
    {
        const ComponentID compID = ComponentID( ch );
        quantizeFrame(qp, originalImage, quantizedImage, encStruct, compID, bitDepths, maxLog2TrDynamicRange);  // give the whole extended image ( .getOrigBuf() and .getRecoBuf() ) to the function because vector icues_enc is based on extended frames
    }
}



void quantizeFrame(const int qp, const PelUnitBuf &originalImage, PelUnitBuf &quantizedImage, EncodingStructure &encStruct, const ComponentID compID, const int bitDepths[], int maxLog2TrDynamicRange)
{
    int bit_depth = bitDepths[toChannelType(compID)];

    //MyQpParam cQP(qp, toChannelType(compID), encStruct.vec_icus[0].get_sourceX().chromaFormat, bit_depth, 0, 0);


    //MyTransform2D transform2D;
    //MyQuant quant;
    TCoeff uiAbsSum;

    if(compID==COMPONENT_Y)
        quantizedImage.get(compID).fill(0);
    else
        quantizedImage.get(compID).fill(128);   // yuv=(0,128,128) represents black value

    for(UInt i=0; i < encStruct.vec_icus.size(); i++)
    {
        InteractiveCodingUnit& curr_icu = encStruct.vec_icus[i].get_sourceX();
        for(UInt j=0; j < curr_icu.TUs.size(); j++)
        {
            //FIXME
            //MyTransformUnit& curr_tu = curr_icu.TUs[j];
            //transform2D.transformNxN(curr_tu.getTransformCoeffs(compID), originalImage, curr_tu.block(compID), bit_depth, maxLog2TrDynamicRange);
            //quant.quant(curr_tu.getQuantizedTransformCoeffs(compID), curr_tu.getTransformCoeffs(compID), uiAbsSum, cQP, SliceType::I_SLICE, bit_depth, maxLog2TrDynamicRange);
            //quant.dequant(curr_tu.getInverseQuantizedTransformCoeffs(compID), curr_tu.getQuantizedTransformCoeffs(compID), cQP, bit_depth, maxLog2TrDynamicRange);
            //transform2D.invTransformNxN(quantizedImage, curr_tu.block(compID), curr_tu.getInverseQuantizedTransformCoeffs(compID), bit_depth, maxLog2TrDynamicRange);


        }
    }
}
