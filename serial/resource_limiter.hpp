#ifndef meld_serial_resource_limiter_hpp
#define meld_serial_resource_limiter_hpp

#include "serial/sized_tuple.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <concepts>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace meld {

  // ResourceHandle is the concept that all types that are used as "tokens"
  // must model.
  template <typename T>
  concept ResourceHandle = std::semiregular<T>;

  class default_resource_token {};

  template <ResourceHandle Token = default_resource_token>
  class resource_limiter : public tbb::flow::buffer_node<Token const*> {
  public:
    using token_type = Token const*;

    explicit resource_limiter(tbb::flow::graph& g,
                              std::string const& name,
                              unsigned int n_tokens = 1) :
      tbb::flow::buffer_node<token_type>{g}, name_{name}
    {
      tokens_.reserve(n_tokens);
      for (std::size_t i = 0; i != n_tokens; ++i) {
        tokens_.emplace_back();
      }
    }

    void activate()
    {
      // The serializer must not be activated until it resides in its final resting spot.
      // IOW, if a container of resource limiters grows, the locations of the resource
      // limiters can move around, introducing memory errors if try_put(...) has been
      // attempted in a different location than when it's used during the graph execution.

      // Place tokens into the buffer.
      for (auto const& token : tokens_) {
        tbb::flow::buffer_node<token_type>::try_put(&token);
      }
    }

    auto const& name() const { return name_; }

  private:
    std::string name_;
    std::vector<Token> tokens_;
  };

  // TODO:
  // Consider removing this. That means tests need to directly make their
  // own resource_limiter objects.
  class resource_limiters {
  public:
    explicit resource_limiters(tbb::flow::graph& g);
    void activate();

    auto get(auto... resources) -> sized_tuple<resource_limiter<int>&, sizeof...(resources)>
    {
      // FIXME: Need to make sure there are no duplicates!
      return std::tie(get(std::string(resources))...);
    }

  private:
    resource_limiter<int>& get(std::string const& name);
    tbb::flow::graph& graph_;
    std::unordered_map<std::string, resource_limiter<int>> limiters_;
  };
}

#endif /* meld_serial_resource_limiter_hpp */
