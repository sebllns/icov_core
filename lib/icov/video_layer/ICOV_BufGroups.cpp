#include "ICOV_BufGroups.h"

#include "Buffer.h"
#include "CodingStructure.h"
#include "Common.h"
#include "Unit.h"

icov::video::BufGroups::BufGroups() {}

void icov::video::BufGroups::destroy() {

  for (UInt i = 0; i < NUM_PIC_TYPES; i++) {
    _bufs[i].destroy();
    //    _margins[i] = 0;
  }
  // destroy reconstruction flag buffer
  _recoFlagBuf.destroy();
}

icov::video::BufGroups::~BufGroups() { destroy(); }

void icov::video::BufGroups::create(
    int pictureType, const ChromaFormat &chromaFormat, const Size &lumaSize,
    const unsigned maxCUSize, const unsigned margin, const unsigned alignment,
    const bool scaleChromaMargin) {
  const Area lumaArea = Area(Position(), lumaSize);
  //  CHECK((pictureType < PIC_RECONSTRUCTION || pictureType > NUM_PIC_TYPES),
  //        "picture type not specified");

  _bufs[pictureType].create(chromaFormat, lumaArea, maxCUSize, margin,
                            alignment, scaleChromaMargin);
  _links[pictureType] = pictureType;

  if (pictureType == PIC_RECONSTRUCTION)
    _recoFlagBuf.create(chromaFormat, lumaArea, maxCUSize, margin, alignment,
                        scaleChromaMargin);
}

void icov::video::BufGroups::create(int pictureType, const UnitArea &unit) {
  //  CHECK((pictureType < PIC_RECONSTRUCTION || pictureType > NUM_PIC_TYPES),
  //        "picture type not specified");
  if (!_bufs.count(pictureType)) {
    _bufs[pictureType].create(unit);
    _links[pictureType] = pictureType;

    if (pictureType == PIC_RECONSTRUCTION)
      _recoFlagBuf.create(unit);
  }
}
