#include "catch2/catch_test_macros.hpp"
#include "oneapi/tbb/flow_graph.h"
#include "short_circuiter/short_circuiter_node.hpp"
#include "short_circuiter/simple_short_circuiter_node.hpp"
#include "short_circuiter/timer.hpp"
#include "spdlog/spdlog.h"

#include <functional>
#include <memory>
#include <string>
#include <tuple>

using namespace oneapi::tbb;
using namespace meld;

namespace {
  template <typename T>
  auto printer_for(tbb::flow::graph& g, std::string label)
  {
    return flow::function_node<msg<T>>{
      g, flow::unlimited, [name = std::move(label)](msg<T> const& m) {
        if (m.data) {
          spdlog::debug("{}: {}", name, *m.data);
        }
        else {
          spdlog::debug("{}: empty", name);
        }
        return flow::continue_msg{};
      }};
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
                             return {id, std::make_shared<int>(j)};
                           }
                           return {id, nullptr};
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

TEST_CASE("Simple short circuiting")
{
  meld::timer timer_for_simple{"simply squared numbers"};
  test_short_circuit<simple_short_circuiter<std::tuple<int>, int>>("simply squared numbers");
}

TEST_CASE("Optimized short circuiting")
{
  meld::timer timer_for_optimized{"squared numbers"};
  test_short_circuit<short_circuiter<std::tuple<int>, int>>("squared numbers");
}
