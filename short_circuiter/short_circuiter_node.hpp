#ifndef meld_serial_short_circuiter_node_hpp
#define meld_serial_short_circuiter_node_hpp

#include "oneapi/tbb/flow_graph.h"
#include "short_circuiter/message.hpp"

#include <tuple>
#include <type_traits>

namespace meld {
  template <typename Ins, typename Out>
  class short_circuiter :
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
    short_circuiter(tbb::flow::graph& g, std::size_t concurrency, FT f) :
      base_t{g},
      joiner_{make_join<join_t, input_msgs_t>(g, in_sequence_t{})},
      router_{
        g, tbb::flow::unlimited, [this](msg_tuple_t const& msgs) noexcept { return route(msgs); }},
      wrapped_user_func_{g,
                         concurrency,
                         [func = std::move(f)](msg_tuple_t const& msgs) {
                           return invoke_on_msgs<Out>(func, msgs, in_sequence_t{});
                         }},
      output_{g, tbb::flow::unlimited, [](msg<Out> const& m) noexcept { return m; }}
    {
      make_edge(joiner_, router_);
      make_edge(wrapped_user_func_, output_);

      base_t::set_external_ports(
        specify_input_ports<typename base_t::input_ports_type>(joiner_, in_sequence_t{}),
        typename base_t::output_ports_type{output_});
    }

  private:
    tbb::flow::continue_msg route(msg_tuple_t const& msgs) noexcept
    {
      if (at_least_one_null(msgs)) {
        output_.try_put({std::get<0>(msgs).id, nullptr});
      }
      else {
        wrapped_user_func_.try_put(msgs);
      }
      return {};
    }

    join_t joiner_;
    tbb::flow::function_node<msg_tuple_t, tbb::flow::continue_msg, tbb::flow::lightweight> router_;
    tbb::flow::function_node<msg_tuple_t, msg<Out>> wrapped_user_func_;
    tbb::flow::function_node<msg<Out>, msg<Out>, tbb::flow::lightweight> output_;
  };
}

#endif /* meld_serial_short_circuiter_node_hpp */
