/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/
#ifndef MYINTRAPREDICTION_H
#define MYINTRAPREDICTION_H
#include "CommonLib/IntraPrediction.h"
#include "mystorage.h"



class MyIntraPrediction
{
public:
    enum REFERENCE_SAMPLE_SIDE
    {
        NO_SIDE = 0,
        LEFT               = 0b000001,
        RIGHT              = 0b000010,
        TOP                = 0b000100,
        DOWN               = 0b001000,
        LEFT_TOP      = TOP  | LEFT,    // 0b00101
        LEFT_DOWN     = DOWN | LEFT,    // 0b01001
        RIGHT_DOWN    = DOWN | RIGHT,   // 0b01010
        RIGHT_TOP     = TOP  | RIGHT,   // 0b00101
        LEFT_TOPLEFT_TOP          = LEFT_TOP    | 0b010000,  // it will not be used for the compression, I just added them to emphasize that the corner point is not included in the previous states
        LEFT_DOWNLEFT_DOWN        = LEFT_DOWN   | 0b010000,
        RIGHT_DOWNRIGHT_DOWN      = RIGHT_DOWN  | 0b010000,
        RIGHT_TOPRIGHT_TOP        = RIGHT_TOP   | 0b010000,
        NUM_REFERENCE_SAMPLES_SIDES= 13,
        INTRA_MESH=0b100000,
    };

private:
    Pel* m_piYuvExt[MAX_NUM_COMPONENT][NUM_PRED_BUF];
    Int  m_iYuvExtSize;


    static const UChar m_aucIntraFilter[MAX_NUM_CHANNEL_TYPE][MAX_INTRA_FILTER_DEPTHS];

    CompArea m_lastArea;
    bool m_filterRefSamples;
    bool m_flipVert, m_flipHoriz;
    Int _bitDepth;
protected:
    ChromaFormat  m_currChromaFormat;
    Void destroy();
    void xFillReferenceSamples(const PelUnitBuf &recoUnitBuf, const BoolUnitBuf &flagUnitBufReconstructedPixels , Pel *refBufUnfiltered, const CompArea &area, const Int bitDepths);
    void xFillReferenceSamples(const PelUnitBuf &recoUnitBuf, const REFERENCE_SAMPLE_SIDE &refSampSide          , Pel *refBufUnfiltered, const CompArea &area, const Int bitDepths);
    void xFillReferenceSamples(const PelUnitBuf &recoUnitBuf, const BoolUnitBuf &flagUnitBufReconstructedPixels , const REFERENCE_SAMPLE_SIDE &refSampSide   , Pel *refBufUnfiltered, const CompArea &area, const Int bitDepths);

    void xFilterReferenceSamples ( const Pel* refBufUnfiltered, Pel* refBufFiltered, const CompArea &area);

    /// Function for deriving planar intra prediction. This function derives the prediction samples for planar mode (intra coding). NOTE: Bit-Limit - 24-bit source
    Void xPredIntraPlanar( const CPelBuf &pSrc, PelBuf &pDst);

    Pel  xGetPredValDc( const CPelBuf &pSrc, const Size &dstSize ); // Function for calculating DC value of the reference samples used in Intra prediction. NOTE: Bit-Limit - 25-bit source
    inline Void xPredIntraDc( const CPelBuf &pSrc, PelBuf &pDst) {pDst.fill(xGetPredValDc( pSrc, pDst ));}

    /** Function for deriving the simplified angular intra predictions.
    *
    * This function derives the prediction samples for the angular mode based on the prediction direction indicated by
    * the prediction mode index. The prediction direction is given by the displacement of the bottom row of the block and
    * the reference row above the block in the case of vertical prediction or displacement of the rightmost column
    * of the block and reference column left from the block in the case of the horizontal prediction. The displacement
    * is signalled at 1/32 pixel accuracy. When projection of the predicted pixel falls inbetween reference samples,
    * the predicted value for the pixel is linearly interpolated from the reference samples. All reference samples are taken
    * from the extended main reference.
    */
    Void xPredIntraAng( const CPelBuf &pSrc, PelBuf &pDst, const UInt dirMode);

    Pel*  getPredictorPtr(const ComponentID compID, const Bool bUseFilteredPredictions = false) { return m_piYuvExt[compID][bUseFilteredPredictions?PRED_BUF_FILTERED:PRED_BUF_UNFILTERED]; }
public:
    MyIntraPrediction();
    virtual ~MyIntraPrediction();

    Void init(ChromaFormat chromaFormatIDC);

    /// set parameters from CU data for accessing intra data
    Void initIntraPatternChType(const PelUnitBuf &recoUnitBuf, const BoolUnitBuf &flagUnitBufReconstructedPixels, const CompArea &area, const Int bitDepths, const Bool bFilterRefSamples = false );
    Void initIntraPatternChType(const PelUnitBuf &recoUnitBuf, const REFERENCE_SAMPLE_SIDE &refSampSide, const CompArea &area, const Int bitDepths, const Bool bFilterRefSamples = false );
    Void initIntraPatternChType(const PelUnitBuf &recoUnitBuf, const BoolUnitBuf &flagUnitBufReconstructedPixels, const REFERENCE_SAMPLE_SIDE &refSampSide, const CompArea &area, const Int bitDepths, const Bool bFilterRefSamples = false );

    /** returns an integer which its last 4 least significant bits shows which sides are available. for more information refer to enum REFERENCE_SAMPLE_SIDE
     * Also the bit levels [5-8] (zero-based index) is 1 if the corner point is also available:
     * bit level 4 = (not used) to avoid conflict of corner usage in REFERENCE_SAMPLE_SIDE.
     * bit level 5 = top-left corner
     * bit level 6 = top-right corner
     * bit level 7 = bottom-left corner
     * bit level 8 = bottom-right corner
     */
    int getAvailableSides(const BoolUnitBuf &flagUnitBufReconstructedPixels, const CompArea &area);

    const CPelBuf getTopRowLeftColumnBuffer() const    {return CPelBuf( m_piYuvExt[m_lastArea.compID][m_filterRefSamples], m_lastArea.width+m_lastArea.height+1, m_lastArea.width+m_lastArea.height+1, m_lastArea.width+m_lastArea.height+1 );} // stride is equal to m_lastArea.width+m_lastArea.height+1. check end lines of initIntraPatternChType for more information

    /// apply intra
    void predIntraAng(PelBuf &piPred, const UInt dirMode);
};

#endif // MYINTRAPREDICTION_H
