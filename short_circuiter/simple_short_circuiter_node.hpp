#ifndef meld_serial_simple_short_circuiter_node_hpp
#define meld_serial_simple_short_circuiter_node_hpp

#include "oneapi/tbb/flow_graph.h"
#include "short_circuiter/message.hpp"

#include <functional>
#include <memory>
#include <tuple>

namespace meld {
  template <typename Ins, typename Out>
  class simple_short_circuiter :
    public tbb::flow::composite_node<msg_tuple_from<Ins>, std::tuple<msg<Out>>> {
    static auto const in_size = std::tuple_size_v<Ins>;
    using msg_tuple_t = msg_tuple_from<Ins>;
    using base_t = tbb::flow::composite_node<msg_tuple_t, std::tuple<msg<Out>>>;
    using input_msgs_t =
      std::conditional_t<in_size == 1, msg<std::tuple_element_t<0, Ins>>, msg_tuple_t>;

    using in_sequence_t = std::make_index_sequence<in_size>;

    using join_t = std::conditional_t<
      in_size == 1,
      tbb::flow::function_node<input_msgs_t, msg_tuple_t, tbb::flow::lightweight>,
      tbb::flow::join_node<input_msgs_t, tbb::flow::tag_matching>>;

  public:
    template <typename FT>
    simple_short_circuiter(tbb::flow::graph& g, std::size_t concurrency, FT f) :
      base_t{g},
      joiner_{make_join<join_t, input_msgs_t>(g, in_sequence_t{})},
      wrapped_user_func_{
        g, concurrency, [func = std::move(f)](msg_tuple_t const& msgs) -> msg<Out> {
          if (at_least_one_null(msgs)) {
            return {std::get<0>(msgs).id, nullptr};
          }
          return invoke_on_msgs<Out>(func, msgs, in_sequence_t{});
        }}
    {
      make_edge(joiner_, wrapped_user_func_);

      base_t::set_external_ports(
        specify_input_ports<typename base_t::input_ports_type>(joiner_, in_sequence_t{}),
        typename base_t::output_ports_type{wrapped_user_func_});
    }

  private:
    join_t joiner_;
    tbb::flow::function_node<msg_tuple_t, msg<Out>> wrapped_user_func_;
  };
}

#endif /* meld_serial_simple_short_circuiter_node_hpp */
