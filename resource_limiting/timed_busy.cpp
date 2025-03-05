#include "resource_limiting/timed_busy.hpp"

void meld::timed_busy(std::chrono::milliseconds const& duration)
{
  using namespace std::chrono;
  auto const stop = steady_clock::now() + duration;
  while (steady_clock::now() < stop) {
    // Do nothing
  }
}
