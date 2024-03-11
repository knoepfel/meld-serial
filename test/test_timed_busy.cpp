#include "catch2/catch_template_test_macros.hpp"
#include "catch2/catch_test_macros.hpp"
#include "serial/timed_busy.hpp"

#include <chrono>

TEST_CASE("timed_busy")
{
  auto const start = std::chrono::steady_clock::now();
  meld::timed_busy(std::chrono::milliseconds{20});
  REQUIRE(std::chrono::steady_clock::now() - start > std::chrono::milliseconds{20});
}
