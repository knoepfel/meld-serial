#ifndef meld_serial_short_circuiter_node_hpp
#define meld_serial_short_circuiter_node_hpp

#include "oneapi/tbb/flow_graph.h"

#include <functional>
#include <memory>
#include <tuple>

namespace meld {
  template <typename T>
  using ptr = std::shared_ptr<T>;

  template <typename T>
  struct msg {
    std::size_t id;
    ptr<T> data;
  };

  template <typename... Ts>
  using msg_tuple = std::tuple<msg<Ts>...>;

  template <typename T>
  struct msg_tuple_from_tuple {};

  template <typename... Ts>
  struct msg_tuple_from_tuple<std::tuple<Ts...>> {
    using type = msg_tuple<Ts...>;
  };

  template <typename Tuple>
  using msg_tuple_from = typename msg_tuple_from_tuple<Tuple>::type;

  template <typename T>
  struct MessageHasher {
    std::size_t operator()(msg<int> const& m) const noexcept { return m.id; }
  };

  template <typename RT, typename Input, std::size_t... Is>
  RT make_join(tbb::flow::graph& g, std::index_sequence<Is...>)
  {
    if constexpr (sizeof...(Is) == 1ull) {
      return {g, tbb::flow::unlimited, [](Input const& t) { return std::tuple<Input>{t}; }};
    }
    else {
      return {g, MessageHasher<std::tuple_element_t<Is, Input>>{}...};
    }
  }

  template <typename Tuple, std::size_t... Is>
  bool all_nonnull(Tuple const& ts, std::index_sequence<Is...>)
  {
    return ((std::get<Is>(ts).data != nullptr) && ...);
  }

  template <typename... Ts>
  bool at_least_one_null(std::tuple<Ts...> const& ts)
  {
    return not all_nonnull(ts, std::index_sequence_for<Ts...>{});
  }

  template <typename Out, typename FT, typename MsgTuple, std::size_t... Is>
  msg<Out> invoke_on_msgs(FT& func, MsgTuple const& msgs, std::index_sequence<Is...>)
  {
    static_assert(std::tuple_size_v<MsgTuple> > 0);
    // The message ID is the same for all messages
    std::size_t id = std::get<0>(msgs).id;
    return {id, std::make_shared<Out>(func(*std::get<Is>(msgs).data...))};
  }

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
      router_{g, tbb::flow::unlimited, [this](msg_tuple_t const& msgs) { return route(msgs); }},
      wrapped_user_func_{g,
                         concurrency,
                         [func = std::move(f)](msg_tuple_t const& msgs) {
                           return invoke_on_msgs<Out>(func, msgs, in_sequence_t{});
                         }},
      output_{g, tbb::flow::unlimited, [](msg<Out> const& m) { return m; }}
    {
      make_edge(joiner_, router_);
      make_edge(wrapped_user_func_, output_);

      auto specify_input_ports = []<std::size_t... Is>(auto& joiner, std::index_sequence<Is...>) {
        if constexpr (sizeof...(Is) == 1) {
          return typename base_t::input_ports_type{joiner};
        }
        else {
          return typename base_t::input_ports_type{input_port<Is>(joiner)...};
        }
      };
      base_t::set_external_ports(specify_input_ports(joiner_, in_sequence_t{}),
                                 typename base_t::output_ports_type{output_});
    }

  private:
    tbb::flow::continue_msg route(msg_tuple_t const& msgs)
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
    tbb::flow::function_node<msg_tuple_t> router_;
    tbb::flow::function_node<msg_tuple_t, msg<Out>> wrapped_user_func_;
    tbb::flow::function_node<msg<Out>, msg<Out>, tbb::flow::lightweight> output_;
  };
}

#endif /* meld_serial_short_circuiter_node_hpp */
