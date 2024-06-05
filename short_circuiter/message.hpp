#ifndef meld_serial_message_hpp
#define meld_serial_message_hpp

#include "oneapi/tbb/flow_graph.h"

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

  template <typename InputPorts, typename Joiner, std::size_t... Is>
  InputPorts specify_input_ports(Joiner& joiner, std::index_sequence<Is...>)
  {
    if constexpr (sizeof...(Is) == 1) {
      return {joiner};
    }
    else {
      return {input_port<Is>(joiner)...};
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

}

#endif /* meld_serial_simple_short_circuiter_node_hpp */
