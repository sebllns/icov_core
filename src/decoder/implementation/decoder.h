#ifndef DECODER_H
#define DECODER_H

#include "interface/decoder.h"

class Decoder : public IDecoder
{
public:
  Decoder();
  virtual void decode(/*stream*/);
  ~Decoder();
};

#endif // DECODER_H
