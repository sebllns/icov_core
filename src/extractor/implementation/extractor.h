#ifndef EXTRACTOR_H
#define EXTRACTOR_H

#include "interface/extractor.h"

class Extractor : public IExtractor
{
public:
  Extractor();
  virtual void extract(/*stream*/);
  ~Extractor();
};

#endif // EXTRACTOR_H
