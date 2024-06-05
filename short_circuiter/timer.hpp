#ifndef meld_serial_timer_hpp
#define meld_serial_timer_hpp

#include "spdlog/spdlog.h"

#include <chrono>
#include <string>

namespace meld {
  class timer {
  public:
    timer(std::string label = "") : label_{label.empty() ? "" : " (for " + std::move(label) + ")"}
    {
    }

    ~timer()
    {
      using namespace std::chrono;
      auto dur = duration<double, std::milli>(steady_clock::now() - start_).count();
      spdlog::info("{:6.4f} ms elapsed{}", dur, label_);
    }

  private:
    using timepoint_t = decltype(std::chrono::steady_clock::now());
    std::string label_;
    timepoint_t start_{std::chrono::steady_clock::now()};
  };
}

#endif /* meld_serial_timer_hpp */
