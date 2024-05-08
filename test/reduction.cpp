#include "oneapi/tbb/flow_graph.h"
#include "reduction/reduction_node.hpp"
#include "spdlog/spdlog.h"

#include <atomic>
#include <format>
#include <string>

using namespace oneapi::tbb;
using namespace meld;

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

  auto receiving_node_for = [](tbb::flow::graph& g, std::string label) {
    return flow::function_node<unsigned int, unsigned int>{
      g, flow::unlimited, [name = std::move(label)](unsigned int const i) {
        spdlog::info("{}: {}", name, i);
        return i;
      }};
  };

  auto sum_receiver = receiving_node_for(g, "sum");
  auto product_receiver = receiving_node_for(g, "product");

  make_edge(src, adder);
  make_edge(src, multiplier);
  make_edge(adder, sum_receiver);
  make_edge(multiplier, product_receiver);

  src.activate();
  g.wait_for_all();
}
