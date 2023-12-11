/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/
#include "ICOV_Units.h"

icov::video::TransformUnit::TransformUnit(
    const UnitArea &unit, bool createTransformBuffer,
    bool createQuantizedTransformBuffer,
    bool createInversetQuantizedTransformBuffer)
    : UnitArea(unit), next(nullptr), previous(nullptr) {
  if (createTransformBuffer)
    _Tcoeffs.create(*this);

  if (createQuantizedTransformBuffer) {
    _quantizedTcoeffs.create(*this);
    _previousQTcoeffs.create(*this);
  }

  if (createInversetQuantizedTransformBuffer)
    _inverseQuantizedTcoeffs.create(*this);
}

icov::video::TransformUnit::TransformUnit(
    const ChromaFormat chromaFormat, const Area &lumaArea,
    bool createTransformBuffer, bool createQuantizedTransformBuffer,
    bool createInversetQuantizedTransformBuffer)
    : UnitArea(chromaFormat, lumaArea), next(nullptr), previous(nullptr) {
  if (createTransformBuffer)
    _Tcoeffs.create(*this);

  if (createQuantizedTransformBuffer)
    _quantizedTcoeffs.create(*this);

  if (createInversetQuantizedTransformBuffer)
    _inverseQuantizedTcoeffs.create(*this);
}

icov::video::TransformUnit::~TransformUnit() {
  _Tcoeffs.release();
  _quantizedTcoeffs.release();
  _inverseQuantizedTcoeffs.release();
}

icov::video::InteractiveCodingUnit::InteractiveCodingUnit(
    const UnitArea &unit, bool createTransformBuffer,
    bool createQuantizedTransformBuffer,
    bool createInverseQuantizedTransformBuffer)
    : UnitArea(unit) {
  _needTransformBuffer = createTransformBuffer;
  _needQuantizedTransformBuffer = createQuantizedTransformBuffer;
  _needInverseQuantizedTransformBuffer = createInverseQuantizedTransformBuffer;
}

icov::video::InteractiveCodingUnit::InteractiveCodingUnit(
    const ChromaFormat chromaFormat, const Area &lumaArea,
    bool createTransformBuffer, bool createQuantizedTransformBuffer,
    bool createInverseQuantizedTransformBuffer)
    : UnitArea(chromaFormat, lumaArea) {
  _needTransformBuffer = createTransformBuffer;
  _needQuantizedTransformBuffer = createQuantizedTransformBuffer;
  _needInverseQuantizedTransformBuffer = createInverseQuantizedTransformBuffer;
}

icov::video::TransformUnit *
icov::video::InteractiveCodingUnit::addTU(const UnitArea &unit) {
  if (!this->contains(unit)) // if the Interactive Coding Unit doesn't contain
    // the unit areas
    return nullptr;

  std::map<icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE,
           std::vector<TransformUnit>>::iterator it;
  if (TUs.empty()) {
    UInt nBlocks_x = (this->Y().width + unit.Y().width - 1) / unit.Y().width;
    UInt nBlocks_y = (this->Y().height + unit.Y().height - 1) / unit.Y().height;
    TUs.reserve(nBlocks_x *
                nBlocks_y); // just to reserve TUs, even if it is not correct,
                            // in push_back it will be extended
    TUs.push_back(TransformUnit(unit, needTransformBuffer(),
                                needQuantizedTransformBuffer(),
                                needInverseQuantizedTransformBuffer()));
    return &TUs[0];
  }

  TUs.push_back(TransformUnit(unit, needTransformBuffer(),
                              needQuantizedTransformBuffer(),
                              needInverseQuantizedTransformBuffer()));
  unsigned int lastIndex = TUs.size() - 1;

  TransformUnit &previousTU = TUs[lastIndex - 1];
  TransformUnit &currTU = TUs[lastIndex];

  currTU.previous = &previousTU;
  previousTU.next = &currTU;

  return &currTU;
  /*

    if(!firstTU)
    {
        firstTU = std::make_shared<TransformUnit>(unit);
        lastTU = firstTU;
//        _previousTU = firstTU;
        return firstTU;
    }

    std::shared_ptr<TransformUnit> tu(new TransformUnit(unit));
//    TransformUnit* tu = new TransformUnit(unit);

    tu->previous = lastTU.get();
    lastTU->next = tu.get();

    lastTU = tu;

    return lastTU;
    */
}

icov::video::TransformUnit *
icov::video::InteractiveCodingUnit::getTU(const UnitArea &unit) {
  //    if(!firstTU)
  //        return nullptr;

  //    for(std::shared_ptr<TransformUnit> tu = firstTU; tu;
  //    tu=std::shared_ptr<TransformUnit> (tu->next))
  //    {
  //        if(*tu==unit)
  //            return tu;
  //    }

  //    return nullptr;

  for (unsigned int i = 0; i < TUs.size(); i++) {
    if (TUs[i] == unit)
      return &TUs[i];
  }
  return nullptr;
}

int icov::video::SideInformationUnit::estimateSIRequiredBitsToCodeIntraMode(
    const ComponentID compID, UInt dirMode,
    const icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE &refSampSide)
    const {
  if (refSampSide == icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE::
                         NONE) // When you have no_SIDE this means you are
                               // coding with no prediction (like access block),
                               // so you do not need to signal the intra mode.
    return 0;

  ChannelType chType = toChannelType(compID);

  if (chType == CHANNEL_TYPE_LUMA) {
    return std::ceil(std::log2(NUM_LUMA_MODE));
  } else {

    if (dirMode == intraDir[COMPONENT_Y]) {
      return 1;
    } else {
      int chromaIntraMode = dirMode;

      switch (chromaIntraMode) {
      case PLANAR_IDX:
      case DC_IDX:
      case HOR_IDX:
      case VER_IDX:
        return 3;
        break;
      default:
        throw std::logic_error("intra prediction mode for chroma channels "
                               "should be one of 5 possible values");
        break;
      }
    }
  }
}
