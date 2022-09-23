/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/
#ifndef MYPICTURE_H
#define MYPICTURE_H
#include "CommonLib/Picture.h"
#include "mystorage.h"

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
class MyPicture {

  using const_iterator = std::map<int, PelStorage>::const_iterator;
  using iterator = std::map<int, PelStorage>::iterator;

protected:
  BoolStorage _recoFlagBuf;

  std::map<int, PelStorage> _bufs;
  //  UInt _margins[NUM_PIC_TYPES);

public:
  MyPicture();
  Void destroy();
  ~MyPicture();

  Void create(int pictureType, const ChromaFormat &chromaFormat,
              const Size &lumaSize, const unsigned maxCUSize,
              const unsigned margin, const unsigned alignment = 0,
              const bool scaleChromaMargin = true);
  Void create(int pictureType, const UnitArea &unit);

  inline const_iterator begin() const { return _bufs.begin(); }
  inline const_iterator end() const { return _bufs.end(); }

  inline iterator begin() { return _bufs.begin(); }
  inline iterator end() { return _bufs.end(); }

  PelStorage &get(int pictureType) { return _bufs.at(pictureType); }
  const PelStorage &get(int pictureType) const { return _bufs.at(pictureType); }

  PelUnitBuf &getBuf(int pictureType) { return _bufs.at(pictureType); }
  const CPelUnitBuf getBuf(int pictureType) const {
    return _bufs.at(pictureType);
  }
};

#endif // MYPICTURE_H
