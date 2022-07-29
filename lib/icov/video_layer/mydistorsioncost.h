/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/
#ifndef MYDISTORSIONCOST_H
#define MYDISTORSIONCOST_H
#include "CommonLib/RdCost.h"

class MyDistortionCost
{
private:
    static FpDistFunc m_afpDistortFunc[DF_TOTAL_FUNCTIONS]; // [eDFunc]
    double m_distortionWeight[MAX_NUM_COMPONENT]; // only chroma values are used.

public:
    MyDistortionCost();
    virtual ~MyDistortionCost();
    Distortion getDistPart( const CPelBuf &org, const CPelBuf &cur, Int bitDepth, const ComponentID compID, DFunc eDFunc );

    Void setDistortionChromaWeight(const ComponentID compID, const Double distortionWeight) { m_distortionWeight[compID] = distortionWeight;}
    Void setDistortionChromaWeight(const ComponentID compID, const Int qpy, const ChromaFormat chFmt, const Int chromaQPOffset);

private:

    Void init();
    Void initRdCostX86();

    static Distortion xGetSSEw          ( const DistParam& rcDtParam ); // weighted SSD cost
    static Distortion xGetSSE           ( const DistParam& rcDtParam );
    static Distortion xGetSSE4          ( const DistParam& rcDtParam );
    static Distortion xGetSSE8          ( const DistParam& rcDtParam );
    static Distortion xGetSSE16         ( const DistParam& rcDtParam );
    static Distortion xGetSSE16N        ( const DistParam& rcDtParam );
    static Distortion xGetSSE32         ( const DistParam& rcDtParam );
    static Distortion xGetSSE64         ( const DistParam& rcDtParam );


    static Distortion xGetSAD           ( const DistParam& rcDtParam );
    static Distortion xGetSAD4          ( const DistParam& rcDtParam );
    static Distortion xGetSAD8          ( const DistParam& rcDtParam );
    static Distortion xGetSAD12         ( const DistParam& rcDtParam );
    static Distortion xGetSAD16         ( const DistParam& rcDtParam );
    static Distortion xGetSAD32         ( const DistParam& rcDtParam );
    static Distortion xGetSAD64         ( const DistParam& rcDtParam );
    static Distortion xGetSAD16N        ( const DistParam& rcDtParam );

    static Distortion xGetSAD24         ( const DistParam& rcDtParam );
    static Distortion xGetSAD48         ( const DistParam& rcDtParam );



};

#endif // MYDISTORSIONCOST_H
