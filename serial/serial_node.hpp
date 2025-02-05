#ifndef meld_serial_serial_node_hpp
#define meld_serial_serial_node_hpp

#include "metaprogramming/type_deduction.hpp"
#include "serial/resource_limiter.hpp"
#include "serial/sized_tuple.hpp"

#include "oneapi/tbb/flow_graph.h"

namespace meld {

  //--------------------------------------------------------------------------
  // maybe_wrap_as_tuple wraps a type T into a tuple if it is not already a
  // tuple. The nested type tuple is and std::tuple<T>, regardless of whether
  // T is a tuple or not.
  //
  // maybe_wrap_as_tuple_t<T> is a convenience alias for the nested type.

  // General case: wrap T into a tuple
  template <typename T>
  struct maybe_wrap_as_tuple {
    using type = std::tuple<T>;
  };

  // Special case: T is already a tuple
  template <typename... Ts>
  struct maybe_wrap_as_tuple<std::tuple<Ts...>> {
    using type = std::tuple<Ts...>;
  };

  // Convenience alias
  template <typename T>
  using maybe_wrap_as_tuple_t = typename maybe_wrap_as_tuple<T>::type;

  //--------------------------------------------------------------------------
  // class serial_node<Input, N, Output> is a composite_node<Input, Output>
  // that also waits to obtain a collection of N tokens of type token_t
  // from several serializers.
  // base<InputTuple, OutputTuple> is a an alias for a composite_node
  // with the given input and output tuple types.
  template <typename InputTuple, typename OutputTuple>
  using base = tbb::flow::composite_node<InputTuple, OutputTuple>;

  // This implementation of serial_node uses a bare pointer-to-int as
  // the token type.
  using token_t = int const*;

  template <typename Input, typename Output, typename Resources = std::tuple<>>
  class serial_node : public base<maybe_wrap_as_tuple_t<Input>, maybe_wrap_as_tuple_t<Output>> {
    using input_tuple = maybe_wrap_as_tuple_t<Input>;
    using output_tuple = maybe_wrap_as_tuple_t<Output>;
    using base_t = base<input_tuple, output_tuple>;
    using join_tuple = concatenated_tuples<input_tuple, Resources>;

    template <typename Serializers, std::size_t... I>
    void make_serializer_edges(std::index_sequence<I...>, Serializers const& serializers)
    {
      (make_edge(std::get<I>(serializers), input_port<I + 1>(join_)), ...);
    }

    template <typename Serializers, typename Tuple, std::size_t... I>
    void return_tokens(Serializers serialized_resources [[maybe_unused]],
                       Tuple const& tuple [[maybe_unused]],
                       std::index_sequence<I...>)
    {
      (std::get<I>(serialized_resources).try_put(std::get<I>(tuple)), ...);
    }

    // Private constructor, used in the impelementation of the public constructor.
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
          auto [input, tokens] = pop_head(tup);
          auto output = function(input);
          return_tokens(serialized_resources, tokens, iseq);
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

    /**
    * @brief Constructs a serial_node with a specified flow graph, serializers, and function.
    *
    * This constructor initializes a serial_node that will operate on the given flow graph,
    * use the provided serializers, and execute the specified function. The execution
    * policy is determined based on the number of serializers.
    *
    * @tparam FT The type of the function to be executed.
    * @tparam ResourceLimiters The types of the serializers to be used.
    * @param g The TBB flow graph to which this node will be added.
    * @param serializers A tuple containing references to the serializers.
    * @param f The function to be executed by the node.
    */
    template <typename FT, typename... ResourceLimiters>
    explicit serial_node(tbb::flow::graph& g,
                         std::tuple<ResourceLimiters&...> const& serializers,
                         FT f) :
      serial_node{g,
                  (sizeof...(ResourceLimiters) > 0 ? tbb::flow::serial : tbb::flow::unlimited),
                  std::move(f),
                  serializers,
                  std::make_index_sequence<sizeof...(ResourceLimiters)>{}}
    {
    }

  private:
    tbb::flow::buffer_node<Input> buffered_msgs_;
    tbb::flow::join_node<join_tuple, tbb::flow::reserving> join_;
    tbb::flow::function_node<join_tuple, Output> serialized_function_;
  };

  // Deduction guide.
  // If the user provides a callable type and a tuple of token types, we can deduce
  // the input and output types (and the tuple of resources).
  template <typename FT, typename... Ts>
  serial_node(tbb::flow::graph&, std::tuple<Ts&...>, FT)
    -> serial_node<function_parameter_type<0, FT>,          // Input
                   return_type<FT>,                         // Output
                   std::tuple<typename Ts::token_type...>>; // Resources

  // Deduction guide.
  // If the user provides a callable type and an integer, we can deduce the input
  // and output types. The result is a serial_node that does not serialize on
  // any resources, but has the specified concurrency.
  template <typename FT, std::convertible_to<int> T>
  serial_node(tbb::flow::graph&, T, FT)
    -> serial_node<function_parameter_type<0, FT>, return_type<FT>, std::tuple<>>;
}

#endif /* meld_serial_serial_node_hpp */
