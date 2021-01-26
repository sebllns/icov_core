#ifndef IRENDERER_H
#define IRENDERER_H

#include "request.h"
#include "common.h"

class IRenderer
{
protected:
  IRequest *m_request;
public:
  IRenderer();
  virtual void drawPixels(/*pixels*/) = 0;
  void setRequest(IRequest*);
  const IRequest& getRequest() const;
};

#endif // IRENDERER_H
