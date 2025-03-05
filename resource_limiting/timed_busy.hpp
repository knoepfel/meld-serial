#ifndef serial_timed_busy_hpp
#define serial_timed_busy_hpp

#include <chrono>

namespace meld {
  void timed_busy(std::chrono::milliseconds const& duration);
}
#endif
