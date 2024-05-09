#include "oneapi/tbb/flow_graph.h"
#include "reduction/reduction_node.hpp"
#include "spdlog/spdlog.h"

#include <atomic>
#include <format>
#include <string>

using namespace oneapi::tbb;
using namespace meld;

namespace {
  template <typename T>
  auto printer_for = [](tbb::flow::graph& g, std::string label) {
    return flow::function_node<T>{g, flow::unlimited, [name = std::move(label)](T const& t) {
                                    spdlog::info("{}: {}", name, t);
                                    return flow::continue_msg{};
                                  }};
  };
}

int main()
{
  flow::graph g;
  unsigned int i{};
  flow::input_node src{g, [&i](flow_control& fc) -> reduction_msg<unsigned int> {
                         if (i < 10) {
                           return reduction_tag<unsigned int>{0, ++i};
                         }
                         else if (i == 10) {
                           return reduction_end{0, i++};
                         }
                         fc.stop();
                         return {};
                       }};

  reduction_node<unsigned int, std::atomic<unsigned int>> adder{
    g, flow::unlimited, 0, [](std::atomic<unsigned int>& result, unsigned int const i) {
      result += i;
    }};

  reduction_node<unsigned int, unsigned int> multiplier{
    g, flow::serial, 1, [](unsigned int& result, unsigned int const i) { result *= i; }};

  reduction_node<unsigned int, std::string> dasher{
    g, flow::serial, "", [](std::string& result, unsigned int const i) {
      if (!result.empty()) {
        result += "-";
      }
      result += std::to_string(i);
    }};

  auto sum_printer = printer_for<unsigned int>(g, "sum");
  auto product_printer = printer_for<unsigned int>(g, "product");
  auto dashed_printer = printer_for<std::string>(g, "dash-separated string");

  make_edge(src, adder);
  make_edge(src, multiplier);
  make_edge(src, dasher);
  make_edge(adder, sum_printer);
  make_edge(multiplier, product_printer);
  make_edge(dasher, dashed_printer);

  src.activate();
  g.wait_for_all();
}
