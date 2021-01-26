#include "./decoder.h"

Decoder::Decoder()
{
  cout << "new Decoder" << endl;
}

Decoder::~Decoder()
{
  cout << "destroy Decoder" << endl;
}


void Decoder::decode()
{
  cout<<"Decoder::decode"<<endl;
  m_renderer -> drawPixels();
}

