/// \todo remove this after the test is done
#pragma warning(disable : 4244)

#include "BlockDecoder.h"
#include <cmath>

using namespace std;

BlockDecoder::BlockDecoder() {
  cout << "new Decoder" << endl;
  m_startTime = clock();
}

BlockDecoder::~BlockDecoder() { cout << "destroy Decoder" << endl; }

void BlockDecoder::decode() {
  cout << "Decoder::decode" << endl;

  int width = m_width;
  int height = m_height;
  unsigned char *dst = (unsigned char *)m_pixels;

  float t = getTime();

  for (int y = 0; y < height; ++y) {
    unsigned char *ptr = dst;
    for (int x = 0; x < width; ++x) {

      // Write the texture pixel
      // ptr[0] = 127.0f + (xAngle / 360.0f) * 127.0f;
      // ptr[1] = 127.0f + (yAngle / 360.0f) * 127.0f;
      // ptr[2] = 0.0f;

      ptr[0] = 0.0f;
      ptr[1] = 0.0f;
      ptr[2] = 0.0f;
      ptr[3] = 0.0f;

      if (x % 128 == 0 || y % 128 == 0) {
        ptr[1] = 255.0f;
        ptr[3] = 255.0f;
      }

      if (x > 0.4f * width && x < 0.6f * width && y > 0.4f * height &&
          y < 0.6f * height) {
        ptr[2] = ptr[3] = 127.0f * 0.5f * (1 + sinf(0.8f * t));
      }

      // To next pixel (our pixels are 4 bpp)
      ptr += 4;
    }

    // To next image row
    dst += m_rowPitch;
  }

  m_renderer->drawPixels(m_pixels, m_width, m_height);
}

float BlockDecoder::getTime() {
  return (float)(clock() - m_startTime) / CLOCKS_PER_SEC;
}

void BlockDecoder::processBlock(const Block &b, uint16_t prediction) {
  std::cout << "decode block " << b.getId(Block::SELF) << std::endl;
}
