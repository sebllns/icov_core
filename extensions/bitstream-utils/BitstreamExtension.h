#ifndef BITSTREAMEXTENSION_H
#define BITSTREAMEXTENSION_H
#pragma once

#include <boost/crc.hpp>
#include "common.h"

namespace icov
{
	namespace BitstreamExtension
	{

		typedef int checksum_t;

		checksum_t GetCheckSum(const byte *buffer, int length)
		{
			boost::crc_16_type result;
			result.process_bytes(buffer, length);
			return (checksum_t)result.checksum();
		}

		bool ChecksumEqual(checksum_t a, checksum_t b)
		{
			return a==b;
		}

		int ChecksumDiff(checksum_t a, checksum_t b)
		{
			return (int)(a - b);
		}

	}
}
#endif