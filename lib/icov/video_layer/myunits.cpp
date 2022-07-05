/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/
#include "myunits.h"


MyTransformUnit::MyTransformUnit(const UnitArea &unit, bool createTransformBuffer, bool createQuantizedTransformBuffer, bool createInversetQuantizedTransformBuffer) : UnitArea(unit), next(nullptr), previous(nullptr)
{
    if(createTransformBuffer)
        _Tcoeffs.create(*this);

    if(createQuantizedTransformBuffer)
        _quantizedTcoeffs.create(*this);

    if(createInversetQuantizedTransformBuffer)
        _inverseQuantizedTcoeffs.create(*this);
}

MyTransformUnit::MyTransformUnit(const ChromaFormat chromaFormat, const Area &lumaArea, bool createTransformBuffer, bool createQuantizedTransformBuffer, bool createInversetQuantizedTransformBuffer) : UnitArea(chromaFormat, lumaArea), next(nullptr), previous(nullptr)
{
    if(createTransformBuffer)
        _Tcoeffs.create(*this);

    if(createQuantizedTransformBuffer)
        _quantizedTcoeffs.create(*this);

    if(createInversetQuantizedTransformBuffer)
        _inverseQuantizedTcoeffs.create(*this);
}

MyTransformUnit::~MyTransformUnit()
{
    _Tcoeffs.release();
    _quantizedTcoeffs.release();
    _inverseQuantizedTcoeffs.release();
}


InteractiveCodingUnit::InteractiveCodingUnit(const UnitArea &unit, bool createTransformBuffer, bool createQuantizedTransformBuffer, bool createInverseQuantizedTransformBuffer) : UnitArea(unit)
{
    _needTransformBuffer= createTransformBuffer;
    _needQuantizedTransformBuffer= createQuantizedTransformBuffer;
    _needInverseQuantizedTransformBuffer= createInverseQuantizedTransformBuffer;
}

InteractiveCodingUnit::InteractiveCodingUnit(const ChromaFormat chromaFormat, const Area &lumaArea, bool createTransformBuffer, bool createQuantizedTransformBuffer, bool createInverseQuantizedTransformBuffer) : UnitArea(chromaFormat, lumaArea)
{
    _needTransformBuffer= createTransformBuffer;
    _needQuantizedTransformBuffer= createQuantizedTransformBuffer;
    _needInverseQuantizedTransformBuffer= createInverseQuantizedTransformBuffer;
}

MyTransformUnit* InteractiveCodingUnit::addTU(const UnitArea &unit)
{
    if(! this->contains(unit))  // if the Interactive Coding Unit doesn't contain the unit areas
        return nullptr;

    std::map< MyIntraPrediction::REFERENCE_SAMPLE_SIDE, std::vector<MyTransformUnit> >::iterator it;
    if(TUs.empty())
    {
        UInt nBlocks_x = (this->Y().width  + unit.Y().width  - 1)/unit.Y().width;
        UInt nBlocks_y = (this->Y().height + unit.Y().height - 1)/unit.Y().height;
        TUs.reserve(nBlocks_x * nBlocks_y); // just to reserve TUs, even if it is not correct, in push_back it will be extended
        TUs.push_back(MyTransformUnit(unit, needTransformBuffer(), needQuantizedTransformBuffer(), needInverseQuantizedTransformBuffer() ));
        return &TUs[0];
    }

    TUs.push_back(MyTransformUnit(unit, needTransformBuffer(), needQuantizedTransformBuffer(), needInverseQuantizedTransformBuffer() ));
    unsigned int lastIndex =TUs.size() - 1;

    MyTransformUnit& previousTU = TUs[lastIndex-1];
    MyTransformUnit& currTU     = TUs[lastIndex];

    currTU.previous = &previousTU;
    previousTU.next = &currTU;

    return &currTU;
}

MyTransformUnit* InteractiveCodingUnit::getTU(const UnitArea &unit)
{

    for(unsigned int i=0; i < TUs.size(); i++)
    {
        if(TUs[i]==unit)
            return &TUs[i];
    }
    return nullptr;
}

