#ifndef IEXTRACTOR_H
#define IEXTRACTOR_H

#include "common.h"

class IExtractor
{
protected:

public:
  virtual ~IExtractor(){}
  virtual void extract(/*stream*/) = 0;
  /*void setRenderer(IRenderer*);
  const IRenderer& getRenderer() const;*/
};

#endif // IEXTRACTOR_H
