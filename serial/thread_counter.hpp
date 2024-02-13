#ifndef meld_serial_thread_counter_hpp
#define meld_serial_thread_counter_hpp

#include <atomic>
#include <stdexcept>
#include <string>

namespace meld {
  class thread_counter {
  public:
    using counter_type = std::atomic<unsigned int>;
    using value_type = counter_type::value_type;
    thread_counter(counter_type& counter, value_type const max_value = 1) :
      counter_{counter}, max_{max_value}
    {
      auto const count = ++counter_;
      if (count > max_) {
        throw std::runtime_error("Too many threads encountered: " + std::to_string(count) +
                                 " vs. max allowed of " + std::to_string(max_));
      }
    }
    ~thread_counter() { --counter_; }

  private:
    counter_type& counter_;
    value_type max_;
  };
}

#endif /* meld_serial_thread_counter_hpp */
