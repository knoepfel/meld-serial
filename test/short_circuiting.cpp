#include "catch2/catch_test_macros.hpp"
#include "oneapi/tbb/flow_graph.h"
#include "resource_limiting/timed_busy.hpp"
#include "short_circuiter/short_circuiter_node.hpp"
#include "short_circuiter/simple_short_circuiter_node.hpp"
#include "short_circuiter/timer.hpp"
#include "spdlog/spdlog.h"

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>

using namespace oneapi::tbb;
using namespace meld;
using namespace std::chrono_literals;

namespace {
  template <typename T>
  flow::function_node<msg<T>> printer_for(tbb::flow::graph& g, std::string label)
  {
    auto maybe_print_data = [name = std::move(label)](msg<T> const& m) {
      if (m.data) {
        spdlog::info("{}: {}", name, *m.data);
      }
      return flow::continue_msg{};
    };
    return {g, flow::unlimited, maybe_print_data};
  }

  template <typename ShortCircuiter>
  void test_short_circuit(std::string const& printer_label)
  {
    flow::graph g;
    int i{};
    flow::input_node src{g, [&i](flow_control& fc) -> msg<int> {
                           if (i == 10) {
                             fc.stop();
                             return {};
                           }

                           auto j = i++;
                           unsigned int id = j;
                           if (j % 2 == 0) {
                             return make_message(id, j);
                           }
                           return make_null_message<int>(id);
                         }};

    auto user_func = [](int const& n) { return n * n; };
    auto printer = printer_for<int>(g, "printer for " + printer_label);
    ShortCircuiter squarer{g, flow::serial, user_func};

    make_edge(src, squarer);
    make_edge(squarer, printer);

    src.activate();
    g.wait_for_all();
  }
}

TEST_CASE("Warmup")
{
  // Purpose of this test is to get any one-time initialization tasks out of the way
  // before doing the real tests.
  spdlog::info("");
  flow::graph g;
}

TEST_CASE("Optimized short circuiting")
{
  spdlog::info("");
  timer report_for_optimized{"squared numbers"};
  test_short_circuit<short_circuiter<std::tuple<int>, int>>("squared numbers");
}

TEST_CASE("Simple short circuiting")
{
  spdlog::info("");
  timer report_for_simple{"simply squared numbers"};
  test_short_circuit<simple_short_circuiter<std::tuple<int>, int>>("simply squared numbers");
}
