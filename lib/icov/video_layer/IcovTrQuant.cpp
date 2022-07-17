#include "IcovTrQuant.h"


void IcovTrQuant::quantizeFrame(const int qp, const PelUnitBuf &originalImage, PelUnitBuf &quantizedImage, EncodingStructure &encStruct, const ComponentID compID, const int bitDepths[], int maxLog2TrDynamicRange)
{
    int bit_depth = bitDepths[toChannelType(compID)];



    TCoeff uiAbsSum; //??
    Ctx ctx; //??
    bool loadTr = false; //??

    for(UInt i=0; i < encStruct.vec_icus.size(); i++)
    {
        InteractiveCodingUnit& curr_icu = encStruct.getICU(i).get_sourceX();
        for(UInt j=0; j < curr_icu.TUs.size(); j++)
        {
            MyTransformUnit& curr_tu = curr_icu.TUs[j];
            TransformUnit &vvc_tu = curr_tu.vvc();

            QpParam cQP(vvc_tu, compID, qp, true /*false ??*/);

//            void invTransformNxN  (TransformUnit &tu, const ComponentID &compID, PelBuf &pResi, const QpParam &cQPs);
//            void transformNxN     ( TransformUnit& tu, const ComponentID& compID, const QpParam& cQP, std::vector<TrMode>* trModes, const int maxCand );
//            void transformNxN     ( TransformUnit& tu, const ComponentID& compID, const QpParam& cQP, TCoeff& uiAbsSum, const Ctx& ctx, const bool loadTr = false );



            m_pVvcTrQuant->transformNxN(vvc_tu, compID, cQP, uiAbsSum, ctx, loadTr);


            //navid
//            m_pVvcTrQuant.transformNxN(curr_tu, compID, originalImage, curr_tu.block(compID), bit_depth, maxLog2TrDynamicRange);
//            quant.quant(curr_tu.getQuantizedTransformCoeffs(compID), curr_tu.getTransformCoeffs(compID), uiAbsSum, cQP, SliceType::I_SLICE, bit_depth, maxLog2TrDynamicRange);
//            quant.dequant(curr_tu.getInverseQuantizedTransformCoeffs(compID), curr_tu.getQuantizedTransformCoeffs(compID), cQP, bit_depth, maxLog2TrDynamicRange);
//            transform2D.invTransformNxN(quantizedImage, curr_tu.block(compID), curr_tu.getInverseQuantizedTransformCoeffs(compID), bit_depth, maxLog2TrDynamicRange);


        }
    }

}
