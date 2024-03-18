#include "serial/serial_node.hpp"
#include "serial/thread_counter.hpp"

#include "oneapi/tbb/flow_graph.h"
#include "spdlog/spdlog.h"
#include "fmt/chrono.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <string>

using namespace meld;
using namespace oneapi::tbb;

namespace {
  auto output_node(tbb::flow::graph& g, std::string const& label)
  {
    return flow::function_node<unsigned int, unsigned int>{
      g, flow::unlimited, [label](unsigned int const i) {
        spdlog::info("Output node {} task {}", label, i);
        return i;
      }};
  }

  template <typename Input, typename Output>
  class algorithm {
  public:
    algorithm(std::chrono::seconds s) : seconds_{s} {}
    Output operator()(Input) const
    {
      auto const begin = std::chrono::steady_clock::now();
      auto t = begin;
      while (t - begin < seconds_) {
        t = std::chrono::steady_clock::now();
      }
      spdlog::info("Executed in {} seconds", t - begin);
      return {};
    }

  private:
    std::chrono::seconds seconds_;
  };
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
