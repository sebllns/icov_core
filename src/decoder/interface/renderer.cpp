#include "renderer.h"

IRenderer::IRenderer()
{

}

void IRenderer::setRequest(IRequest *r)
{
  m_request = r;
}

const IRequest &IRenderer::getRequest() const
{
  return *m_request;
}
