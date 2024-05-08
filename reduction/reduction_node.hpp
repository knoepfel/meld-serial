#ifndef meld_serial_reduction_node_hpp
#define meld_serial_reduction_node_hpp

#include "oneapi/tbb/concurrent_hash_map.h"
#include "oneapi/tbb/flow_graph.h"

#include <atomic>
#include <format>
#include <iostream>
#include <string>
#include <tuple>
#include <variant>

using namespace oneapi::tbb;

namespace meld {
  template <typename T>
  struct reduction_tag {
    std::size_t sequence_id;
    T data;
  };

  struct reduction_end {
    std::size_t sequence_id;
    long long count;
  };

  template <typename T>
  struct maybe_value_type {
    using type = T;
  };

  template <typename T>
  concept has_value_type = requires(T) { typename T::value_type; };

  template <has_value_type T>
  struct maybe_value_type<T> {
    using type = typename T::value_type;
  };

  template <typename T>
  using maybe_value_type_t = typename maybe_value_type<T>::type;

  using count_t = std::atomic<long long>;

  template <typename Input>
  using reduction_msg = std::variant<reduction_tag<Input>, reduction_end>;

  template <typename T>
  T output(T const& t)
  {
    return t;
  }

  template <typename T>
  T output(std::atomic<T> const& result)
  {
    return result.load();
  }

  template <typename Input, typename Result>
  class reduction_node :
    public flow::multifunction_node<reduction_msg<Input>, std::tuple<maybe_value_type_t<Result>>> {
    using output_t = maybe_value_type_t<Result>;
    struct data_with_count {
      data_with_count(output_t initializer) : result{std::move(initializer)} {}
      Result result;
      count_t count{};
      long long total{};
    };
    using data_with_count_ptr = std::shared_ptr<data_with_count>;

    using results_t = tbb::concurrent_hash_map<std::size_t, data_with_count_ptr>;
    using accessor = results_t::accessor;
    using const_accessor = results_t::const_accessor;

  public:
    using base_t = flow::multifunction_node<reduction_msg<Input>, std::tuple<output_t>>;
    using input_msg = reduction_msg<Input>;
    template <typename Initializer, typename Operation>
    reduction_node(flow::graph& g, std::size_t concurrency, Initializer initializer, Operation f) :
      base_t{g,
             concurrency,
             [this, op = std::move(f)](input_msg const& msg, auto& output_ports) {
               count_t* count = nullptr;
               Result* result = nullptr;
               std::size_t sequence_id = -1ull;
               if (auto const* it = std::get_if<reduction_tag<Input>>(&msg)) {
                 sequence_id = it->sequence_id;
                 std::tie(count, std::ignore, result) = result_entry(sequence_id);
                 op(*result, it->data);
                 ++(*count);
               }
               else {
                 auto& end = std::get<reduction_end>(msg);
                 sequence_id = end.sequence_id;
                 long long* n = nullptr;
                 std::tie(count, n, result) = result_entry(sequence_id);
                 *n = end.count;
                 *count -= end.count;
               }
               if (*count == 0) {
                 auto output_result = output(std::as_const(*result));
                 std::get<0>(output_ports).try_put(output_result);
                 results_.erase(sequence_id);
               }
             }},
      initializer_(initializer)
    {
    }

  private:
    std::tuple<count_t*, long long*, Result*> result_entry(std::size_t sequence_id)
    {
      accessor a;
      if (results_.insert(a, sequence_id)) {
        a->second = std::make_shared<data_with_count>(initializer_);
      }
      return {&a->second->count, &a->second->total, &a->second->result};
    }

    output_t initializer_;
    results_t results_;
  };

}

#endif /* meld_serial_reduction_node_hpp */
