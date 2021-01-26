#ifndef IDECODER_H
#define IDECODER_H

#include "common.h"
#include "renderer.h"

class IDecoder
{
protected:
  std::unique_ptr<IRenderer> m_renderer;
public:
  virtual ~IDecoder(){}
  virtual void decode(/*stream*/) = 0;
  void setRenderer(IRenderer*);
  const IRenderer& getRenderer() const;
};

#endif // IDECODER_H
