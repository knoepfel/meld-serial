#include "serial/resource_limiter.hpp"

#include <ranges>

namespace meld {
  resource_limiters::resource_limiters(tbb::flow::graph& g) : graph_{g} {}

  resource_limiter<int>& resource_limiters::get(std::string const& name)
  {
    return limiters_.try_emplace(name, resource_limiter<int>{graph_, name}).first->second;
  }

  void resource_limiters::activate()
  {
    for (auto& serializer : limiters_ | std::views::values) {
      serializer.activate();
    }
  }
}
