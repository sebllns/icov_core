/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/
#ifndef MYUNITS_H
#define MYUNITS_H
#include "CommonLib/Unit.h"
#include "mystorage.h"
#include "myintraprediction.h"
#include <memory>

struct MyTransformUnit : public UnitArea
{
public:

    //TO BE INITIALIZED
    TransformUnit *vvc_tu;
    TransformUnit& vvc()
    {
        return *vvc_tu;
    }

    MyTransformUnit(const UnitArea& unit, bool createTransformBuffer, bool createQuantizedTransformBuffer, bool createInversetQuantizedTransformBuffer);
    MyTransformUnit(const ChromaFormat chromaFormat, const Area &lumaArea, bool createTransformBuffer, bool createQuantizedTransformBuffer, bool createInversetQuantizedTransformBuffer);
    ~MyTransformUnit();

//    std::shared_ptr<MyTransformUnit> next, previous;
    MyTransformUnit *next, *previous;

             CoeffBuf& getTransformCoeffs(const ComponentID id)         {return _Tcoeffs.getBuf(id);}
    const   CCoeffBuf getTransformCoeffs(const ComponentID id) const   {return _Tcoeffs.getBuf(id);}

             CoeffBuf& getQuantizedTransformCoeffs(const ComponentID id)         {return _quantizedTcoeffs.getBuf(id);}
    const   CCoeffBuf getQuantizedTransformCoeffs(const ComponentID id) const   {return _quantizedTcoeffs.getBuf(id);}

             CoeffBuf& getInverseQuantizedTransformCoeffs(const ComponentID id)         {return _inverseQuantizedTcoeffs.getBuf(id);}
    const   CCoeffBuf getInverseQuantizedTransformCoeffs(const ComponentID id) const   {return _inverseQuantizedTcoeffs.getBuf(id);}

protected:
    TCoeffStorage _Tcoeffs;
    QTCoeffStorage _quantizedTcoeffs;
    IQTCoeffStorage _inverseQuantizedTcoeffs;

};


struct InteractiveCodingUnit : public UnitArea
{


    InteractiveCodingUnit(const UnitArea& unit, bool createTransformBuffer = true, bool createQuantizedTransformBuffer=true, bool createInverseQuantizedTransformBuffer= true);
    InteractiveCodingUnit(const ChromaFormat chromaFormat, const Area &lumaArea, bool createTransformBuffer = true, bool createQuantizedTransformBuffer=true, bool createInverseQuantizedTransformBuffer= true);

    std::vector<MyTransformUnit> TUs;

    MyTransformUnit* addTU(const UnitArea& unit);
    MyTransformUnit* getTU(const UnitArea& unit);

    bool needTransformBuffer() {return _needTransformBuffer;}
    bool needQuantizedTransformBuffer() { return _needQuantizedTransformBuffer;}
    bool needInverseQuantizedTransformBuffer() { return _needInverseQuantizedTransformBuffer;}

protected:
    bool _needTransformBuffer, _needQuantizedTransformBuffer, _needInverseQuantizedTransformBuffer;

};

struct SideInformationUnit : public InteractiveCodingUnit
{
    SideInformationUnit(const UnitArea& unit, bool createTransformBuffer = true, bool createQuantizedTransformBuffer=false, bool createInverseQuantizedTransformBuffer= false) :  InteractiveCodingUnit(unit, createTransformBuffer, createQuantizedTransformBuffer, createInverseQuantizedTransformBuffer){}
    SideInformationUnit(const ChromaFormat chromaFormat, const Area &lumaArea, bool createTransformBuffer = true, bool createQuantizedTransformBuffer=false, bool createInverseQuantizedTransformBuffer= false) : InteractiveCodingUnit(chromaFormat, lumaArea, createTransformBuffer, createQuantizedTransformBuffer, createInverseQuantizedTransformBuffer) {}

    UChar intraDir[MAX_NUM_COMPONENT];  // intra prediction direction mode
    std::map<UInt, UInt> bitsToDecode[MAX_NUM_COMPONENT];  // bits needed for decoding per band. key represents band index and value represents number of bits

};

#endif // MYUNITS_H
