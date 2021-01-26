#include "./decoder.h"

void IDecoder::setRenderer(IRenderer* r)
{
  cout << "Decoder::setRenderer" << endl;
  m_renderer = std::unique_ptr<IRenderer>(r);
}


const IRenderer& IDecoder::getRenderer() const
{
  return *m_renderer;
}
