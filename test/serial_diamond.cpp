#include "serial/serial_node.hpp"
#include "serial/thread_counter.hpp"
#include "serial/timed_busy.hpp"

#include "oneapi/tbb/flow_graph.h"
#include "spdlog/spdlog.h"

#include <atomic>
#include <iostream>
#include <string>

using namespace std::chrono_literals;
using namespace meld;
using namespace oneapi::tbb;
void serialize_functions_when_splitting_then_merging()
{
  std::cout << "\nSerialize functions when splitting then merging\n" << std::endl;
  flow::graph g;
  flow::input_node src{g, [i = 0u](flow_control& fc) mutable {
                         if (i < 10u) {
                           return ++i;
                         }
                         fc.stop();
                         return 0u;
                       }};

  std::atomic<unsigned int> root_counter{};
  resource_limiter root_limiter{g, 1};

  auto serial_node_for = [&root_limiter, &root_counter](auto& g, int label) {
    return serial_node{
      g, std::tie(root_limiter), [&root_counter, label](unsigned int const i) -> unsigned int {
        thread_counter c{root_counter};
        spdlog::info("Processing from node {} {}", label, i);
        return i;
      }};
  };

  auto node1 = serial_node_for(g, 1);
  auto node2 = serial_node_for(g, 2);
  auto node3 = serial_node_for(g, 3);

  make_edge(src, node1);
  make_edge(src, node2);
  make_edge(node1, node3);
  make_edge(node2, node3);

  root_limiter.activate();
  src.activate();

  g.wait_for_all();
}

int main() { serialize_functions_when_splitting_then_merging(); }
