#pragma once

#include "icov_assert.h"
#include <system_error>

class IcovErrorCode
{
public:
  static std::error_code last;

  static void check(std::error_code ec)
  {
    if (ec) {
      last = ec;
    }
  }

  static void check_hard(std::error_code ec)
  {
    check(ec);
    ICOV_ASSERT_MSG(!ec, "Error code: " + std::to_string(ec.value()))
  }
};

std::error_code IcovErrorCode::last = std::error_code();
