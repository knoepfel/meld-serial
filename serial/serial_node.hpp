#ifndef meld_serial_serial_node_hpp
#define meld_serial_serial_node_hpp

#include "serial/serializer_node.hpp"
#include "serial/sized_tuple.hpp"

#include "oneapi/tbb/flow_graph.h"

namespace meld {

  template <typename T>
  struct maybe_wrap_as_tuple {
    using type = std::tuple<T>;
  };

  template <typename... Ts>
  struct maybe_wrap_as_tuple<std::tuple<Ts...>> {
    using type = std::tuple<Ts...>;
  };

  template <typename T>
  using maybe_wrap_as_tuple_t = typename maybe_wrap_as_tuple<T>::type;

  template <typename InputTuple, typename OutputTuple>
  using base = tbb::flow::composite_node<InputTuple, OutputTuple>;

  template <typename Input, std::size_t N, typename Output = Input>
  class serial_node : public base<maybe_wrap_as_tuple_t<Input>, maybe_wrap_as_tuple_t<Output>> {
    using input_tuple = maybe_wrap_as_tuple_t<Input>;
    using output_tuple = maybe_wrap_as_tuple_t<Output>;
    using base_t = base<input_tuple, output_tuple>;
    using join_tuple = concatenated_tuples<input_tuple, sized_tuple<token_t, N>>;

    template <typename Serializers, std::size_t... I>
    void make_serializer_edges(std::index_sequence<I...>, Serializers const& serializers)
    {
      (make_edge(std::get<I>(serializers), input_port<I + 1>(join_)), ...);
    }

    template <typename Serializers, std::size_t... I>
    void return_tokens(Serializers serialized_resources [[maybe_unused]], std::index_sequence<I...>)
    {
      // FIXME: Might want to return the received tokens instead of just '1'.
      (std::get<I>(serialized_resources).try_put(1), ...);
    }

    template <typename FT, typename Serializers, std::size_t... I>
    explicit serial_node(tbb::flow::graph& g,
                         std::size_t concurrency,
                         FT f,
                         Serializers serializers,
                         std::index_sequence<I...> iseq) :
      base_t{g},
      buffered_msgs_{g},
      join_{g},
      serialized_function_{
        g,
        concurrency,
        [serialized_resources = std::move(serializers), function = std::move(f), iseq, this](
          join_tuple const& tup) mutable {
          auto input = std::get<0>(tup);
          auto output = function(input);
          return_tokens(serialized_resources, iseq);
          return output;
        }}
    {
      // Need way to route null messages around the join.
      make_edge(buffered_msgs_, input_port<0>(join_));
      make_serializer_edges(iseq, serializers);
      make_edge(join_, serialized_function_);
      base_t::set_external_ports(typename base_t::input_ports_type{buffered_msgs_},
                                 typename base_t::output_ports_type{serialized_function_});
    }

  public:
    template <typename FT>
    explicit serial_node(tbb::flow::graph& g, std::size_t concurrency, FT f) :
      serial_node{g, concurrency, std::move(f), std::tuple{}, std::make_index_sequence<0>{}}
    {
    }

    template <typename FT, typename... Serializers>
    explicit serial_node(tbb::flow::graph& g,
                         std::tuple<Serializers&...> const& serializers,
                         FT f) :
      serial_node{g,
                  (sizeof...(Serializers) > 0 ? tbb::flow::serial : tbb::flow::unlimited),
                  std::move(f),
                  serializers,
                  std::make_index_sequence<sizeof...(Serializers)>{}}
    {
    }

  private:
    tbb::flow::buffer_node<Input> buffered_msgs_;
    tbb::flow::join_node<join_tuple, tbb::flow::reserving> join_;
    tbb::flow::function_node<join_tuple, Output> serialized_function_;
  };
}

#endif /* meld_serial_serial_node_hpp */
