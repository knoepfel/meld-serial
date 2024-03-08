#include "serial/serial_node.hpp"
#include "serial/thread_counter.hpp"

#include "oneapi/tbb/flow_graph.h"
#include "spdlog/spdlog.h"

#include <atomic>
#include <format>
#include <iostream>
#include <string>

using namespace meld;
using namespace oneapi::tbb;

namespace {
  auto output_node(tbb::flow::graph& g, std::string const& label)
  {
    return flow::function_node<unsigned int, unsigned int>{
      g, flow::unlimited, [&label](unsigned int const i) {
        spdlog::info("Output node {} task {}", label, i);
        return i;
      }};
  }
}

void serialize_functions_based_on_resource()
{
  std::cout << "Serialize functions based on resource\n" << std::endl;
  flow::graph g;
  unsigned int i{};
  flow::input_node src{g, [&i](flow_control& fc) {
                         if (i < 10) {
                           return ++i;
                         }
                         fc.stop();
                         return 0u;
                       }};

  serializers serialized_resources{g};

  serial_node<unsigned int, 1> node1{g, serialized_resources.get("ROOT"), [](unsigned int const i) {
                                       spdlog::info("Processing from node 1 {}", i);
                                       return i;
                                     }};

  auto output_1 = output_node(g, "ROOT");

  make_edge(src, node1);

  make_edge(node1, output_1);

  serialized_resources.activate();
  src.activate();
  g.wait_for_all();
}

int main() { serialize_functions_based_on_resource(); }
