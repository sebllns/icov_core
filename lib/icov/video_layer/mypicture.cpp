/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/
#include "mypicture.h"

MyPicture::MyPicture() {}

/*
enum PictureType
{
  PIC_RECONSTRUCTION = 0,
  PIC_ORIGINAL,
  PIC_PREDICTION,
  PIC_RESIDUAL,
  PIC_ORG_RESI,
  NUM_PIC_TYPES
};
 */

Void MyPicture::destroy() {

  for (UInt i = 0; i < NUM_PIC_TYPES; i++) {
    _bufs[i].destroy();
    //    _margins[i] = 0;
  }
  // destroy reconstruction flag buffer
  _recoFlagBuf.destroy();
}

MyPicture::~MyPicture() { destroy(); }

Void MyPicture::create(int pictureType, const ChromaFormat &chromaFormat,
                       const Size &lumaSize, const unsigned maxCUSize,
                       const unsigned margin, const unsigned alignment,
                       const bool scaleChromaMargin) {
  const Area lumaArea = Area(Position(), lumaSize);
  //  CHECK((pictureType < PIC_RECONSTRUCTION || pictureType > NUM_PIC_TYPES),
  //        "picture type not specified");

  _bufs[pictureType].create(chromaFormat, lumaArea, maxCUSize, margin,
                            alignment, scaleChromaMargin);
  //    _margins[pictureType] = margin;

  if (pictureType == PIC_RECONSTRUCTION)
    _recoFlagBuf.create(chromaFormat, lumaArea, maxCUSize, margin, alignment,
                        scaleChromaMargin);
}

Void MyPicture::create(int pictureType, const UnitArea &unit) {
  //  CHECK((pictureType < PIC_RECONSTRUCTION || pictureType > NUM_PIC_TYPES),
  //        "picture type not specified");

  _bufs[pictureType].create(unit);
  //  _margins[pictureType] = 0;

  if (pictureType == PIC_RECONSTRUCTION)
    _recoFlagBuf.create(unit);
}
