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

  // ----------------------------------------------------------------------------
  // default_resource_handle is a handle that controls no resource. It is a stateless
  // object. It is intended to be used to limit access to resources or facilities that
  // do not require any state to describe the resource.
  class default_resource_handle {};

  //----------------------------------------------------------------------------
  // resource_limiter<Handle> is designed to limit the availability
  // of a resource associated with the Handle type.  It does this
  // by managing one or more objects of the type Handle, with the
  // intention that the controlled resource can not be used without a token that
  // points to the handle. The resource_limiter owns the handles, and thus the
  // resources with which they are associated.
  template <typename Handle = default_resource_handle>
  class resource_limiter : public tbb::flow::buffer_node<Handle const*> {
  public:
    // Resources limiters are not copyable but are movable.
    resource_limiter(resource_limiter const&) = delete;
    resource_limiter& operator=(resource_limiter const&) = delete;
    resource_limiter(resource_limiter&&) = default;
    resource_limiter& operator=(resource_limiter&&) = default;

    using token_type = Handle const*;

    /// \brief Constructs a resource_limiter with \a n_tokens tokens of type \a Handle.
    ///
    /// The tokens are stored in a vector and are all "available" when the
    /// resource_limiter is constructed.  The resource_limiter is a
    /// buffer_node<Handle> so that the try_put() function can be used to
    /// "return" a token to the resource_limiter.
    explicit resource_limiter(tbb::flow::graph& g, unsigned int n_handles = 1) :
      tbb::flow::buffer_node<token_type>{g}, handles_(n_handles)
    {
    }

    explicit resource_limiter(tbb::flow::graph& g, std::vector<Handle>&& handles) :
      tbb::flow::buffer_node<token_type>{g}, handles_(std::move(handles))
    {
    }

    /// \brief Activate the resource_limiter by placing one token to each handle
    /// it manages into its buffer.
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
      for (auto const& handle : handles_) {
        tbb::flow::buffer_node<token_type>::try_put(&handle);
      }
    }

  private:
    std::vector<Handle> handles_;
  };
}

#endif /* meld_serial_resource_limiter_hpp */
