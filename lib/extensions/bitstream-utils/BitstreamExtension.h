#pragma once

#include "video_layer/icov_stream.h"
#include <boost/crc.hpp>

namespace icov {
namespace BitstreamExtension {

typedef int checksum_t;

checksum_t GetCheckSum(const byte *buffer, int length) {
  boost::crc_32_type result;
  result.process_bytes(buffer, length);
  return (checksum_t)result.checksum();
}

bool ChecksumEqual(checksum_t a, checksum_t b) { return a == b; }

int ChecksumDiff(checksum_t a, checksum_t b) { return (int)(a - b); }

} // namespace BitstreamExtension
} // namespace icov
