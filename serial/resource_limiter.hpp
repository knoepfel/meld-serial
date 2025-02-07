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

  // default_resource_token models the ResourceHandle concept.
  class default_resource_token {};

  //----------------------------------------------------------------------------
  // resource_limiter<ResourceHandle> is designed to limit the availability
  // of a resource associated with the ResourceHandle type.  It does this
  // by managing one or more tokens of the type ResourceHandle, with the
  // intention that the controlled resource can not be used without a token.
  template <ResourceHandle Token = default_resource_token>
  class resource_limiter : public tbb::flow::buffer_node<Token const*> {
  public:
    using token_type = Token const*;

    /// \brief Constructs a resource_limiter with \a n_tokens tokens of type \a Token.
    ///
    /// The tokens are stored in a vector and are all "available" when the
    /// resource_limiter is constructed.  The resource_limiter is a
    /// buffer_node<Token> so that the try_put() function can be used to
    /// "return" a token to the resource_limiter.
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

    /// \brief Activate the resource_limiter by placing all of the tokens it
    /// manages into its buffer.
    ///
    /// This function must be called before the flow graph is started.  It
    /// is not automatically called by the constructor because the
    /// resource_limiter may be placed into a container (e.g. a vector) that
    /// can grow after the resource_limiter is constructed.  If the
    /// resource_limiter is activated before it is placed into its final
    /// location, then the locations of the tokens in the buffer can become
    /// invalid, resulting in memory errors when the flow graph is executed.
    void activate()
    {
      // Place tokens into the buffer.
      for (auto const& token : tokens_) {
        tbb::flow::buffer_node<token_type>::try_put(&token);
      }
    }

    /// \brief Return the name of the resource_limiter
    /// @return a const reference to the name
    auto const& name() const { return name_; }

  private:
    std::string name_;
    std::vector<Token> tokens_;
  };
}

#endif /* meld_serial_resource_limiter_hpp */
