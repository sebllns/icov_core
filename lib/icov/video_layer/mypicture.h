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
class MyPicture
{
protected:

    BoolStorage _recoFlagBuf;

    PelStorage _bufs[NUM_PIC_TYPES];
    UInt _margins[NUM_PIC_TYPES];
public:
    MyPicture();
    Void destroy();
    ~MyPicture();

    Void create(const PictureType pictureType, const ChromaFormat &chromaFormat, const Size &lumaSize, const unsigned maxCUSize, const unsigned margin, const unsigned alignment = 0, const bool scaleChromaMargin = true);
    Void create(const PictureType pictureType, const UnitArea &unit);



           PelBuf     getOrigBuf(const ComponentID compID)              { return _bufs[PIC_ORIGINAL].getBuf(compID); }
    const CPelBuf     getOrigBuf(const ComponentID compID)  const       { return _bufs[PIC_ORIGINAL].getBuf(compID); }
           PelBuf     getOrigBuf(const CompArea &blk)                   { return _bufs[PIC_ORIGINAL].getBuf(blk);    }
    const CPelBuf     getOrigBuf(const CompArea &blk)       const       { return _bufs[PIC_ORIGINAL].getBuf(blk);    }
           PelUnitBuf getOrigBuf(const UnitArea &unit)                  { return _bufs[PIC_ORIGINAL].getBuf(unit);   }
    const CPelUnitBuf getOrigBuf(const UnitArea &unit)      const       { return _bufs[PIC_ORIGINAL].getBuf(unit);   }
           PelStorage& getOrigBuf()                                     { return _bufs[PIC_ORIGINAL];                }
    const PelStorage& getOrigBuf()                          const       { return _bufs[PIC_ORIGINAL];                }



           PelBuf     getRecoBuf(const ComponentID compID)              { return _bufs[PIC_RECONSTRUCTION].getBuf(compID); }
    const CPelBuf     getRecoBuf(const ComponentID compID)  const       { return _bufs[PIC_RECONSTRUCTION].getBuf(compID); }
           PelBuf     getRecoBuf(const CompArea &blk)                   { return _bufs[PIC_RECONSTRUCTION].getBuf(blk);    }
    const CPelBuf     getRecoBuf(const CompArea &blk)       const       { return _bufs[PIC_RECONSTRUCTION].getBuf(blk);    }
           PelUnitBuf getRecoBuf(const UnitArea &unit)                  { return _bufs[PIC_RECONSTRUCTION].getBuf(unit);   }
    const CPelUnitBuf getRecoBuf(const UnitArea &unit)      const       { return _bufs[PIC_RECONSTRUCTION].getBuf(unit);   }
           PelStorage& getRecoBuf()                                      { return _bufs[PIC_RECONSTRUCTION];                }
    const PelStorage& getRecoBuf()                          const       { return _bufs[PIC_RECONSTRUCTION];                }


           BoolBuf     getRecoFlagBuf(const ComponentID compID)              { return _recoFlagBuf.getBuf(compID); }
    const CBoolBuf     getRecoFlagBuf(const ComponentID compID)  const       { return _recoFlagBuf.getBuf(compID); }
           BoolBuf     getRecoFlagBuf(const CompArea &blk)                   { return _recoFlagBuf.getBuf(blk);    }
    const CBoolBuf     getRecoFlagBuf(const CompArea &blk)       const       { return _recoFlagBuf.getBuf(blk);    }
           BoolUnitBuf getRecoFlagBuf(const UnitArea &unit)                  { return _recoFlagBuf.getBuf(unit);   }
    const CBoolUnitBuf getRecoFlagBuf(const UnitArea &unit)      const       { return _recoFlagBuf.getBuf(unit);   }
           BoolStorage& getRecoFlagBuf()                                      { return _recoFlagBuf;                }
    const BoolStorage& getRecoFlagBuf()                          const       { return _recoFlagBuf;                }



           PelBuf     getPredBuf(const ComponentID compID)              { return _bufs[PIC_PREDICTION].getBuf(compID); }
    const CPelBuf     getPredBuf(const ComponentID compID)  const       { return _bufs[PIC_PREDICTION].getBuf(compID); }
           PelBuf     getPredBuf(const CompArea &blk)                   { return _bufs[PIC_PREDICTION].getBuf(blk);    }
    const CPelBuf     getPredBuf(const CompArea &blk)       const       { return _bufs[PIC_PREDICTION].getBuf(blk);    }
           PelUnitBuf getPredBuf(const UnitArea &unit)                  { return _bufs[PIC_PREDICTION].getBuf(unit);   }
    const CPelUnitBuf getPredBuf(const UnitArea &unit)      const       { return _bufs[PIC_PREDICTION].getBuf(unit);   }
           PelStorage& getPredBuf()                                      { return _bufs[PIC_PREDICTION];                }
    const PelStorage& getPredBuf()                          const       { return _bufs[PIC_PREDICTION];                }



           PelBuf     getBuf(const PictureType &pictureType, const ComponentID compID)              { return _bufs[pictureType].getBuf(compID); }
    const CPelBuf     getBuf(const PictureType &pictureType, const ComponentID compID)  const       { return _bufs[pictureType].getBuf(compID); }
           PelBuf     getBuf(const PictureType &pictureType, const CompArea &blk)                   { return _bufs[pictureType].getBuf(blk);    }
    const CPelBuf     getBuf(const PictureType &pictureType, const CompArea &blk)       const       { return _bufs[pictureType].getBuf(blk);    }
           PelUnitBuf getBuf(const PictureType &pictureType, const UnitArea &unit)                  { return _bufs[pictureType].getBuf(unit);   }
    const CPelUnitBuf getBuf(const PictureType &pictureType, const UnitArea &unit)      const       { return _bufs[pictureType].getBuf(unit);   }
           PelUnitBuf getBuf(const PictureType &pictureType)                                        { return _bufs[pictureType];                }
    const CPelUnitBuf getBuf(const PictureType &pictureType)                            const       { return _bufs[pictureType];                }
};

#endif // MYPICTURE_H
