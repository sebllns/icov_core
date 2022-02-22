#include "Encoder.h"
#include <cmath>

using namespace std;

Encoder::Encoder()
{
  cout << "new encoder" << endl;
}

Encoder::~Encoder()
{
  cout << "destroy encoder" << endl;
}

void Encoder::openFile(const std::string &filename)
{
}

void Encoder::dispose()
{
}

void Encoder::encode(const IPicture &p)
{
}

void Encoder::encodeBlock(const Block &b)
{
}

void Encoder::encodeBlockWithPrediction(const Block &b, uint16_t prediction)
{
}
