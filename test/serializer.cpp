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

void serialize_functions_based_on_resource()
{
  std::cout << "Serialize functions based on resource\n" << std::endl;
  flow::graph g;
  unsigned int i{};
  flow::input_node src{g, [&i](flow_control& fc) {
                         if (i < 10) {
                           auto j = ++i;
                           spdlog::info("-> Emitting {}", j);
                           return j;
                         }
                         spdlog::info("=> Source all done");
                         fc.stop();
                         return 0u;
                       }};

  std::atomic<unsigned int> root_counter{};
  std::atomic<unsigned int> genie_counter{};

  resource_limiter root_limiter{g, "ROOT", 1};
  resource_limiter genie_limiter{g, "GENIE", 1};

  serial_node histogrammer{
    g, std::tie(root_limiter), [&root_counter](unsigned int const i) -> unsigned int {
      thread_counter c{root_counter};
      spdlog::info("Histogramming {}", i);
      timed_busy(10ms);
      return i;
    }};

  serial_node histo_generator{
    g,
    std::tie(root_limiter, genie_limiter),
    [&root_counter, &genie_counter](unsigned int const i) -> unsigned int {
      thread_counter c1{root_counter};
      thread_counter c2{genie_counter};
      spdlog::info("Histo-generating {}", i);
      timed_busy(10ms);
      spdlog::info("Done histo-generating {}", i);
      return i;
    }};

  serial_node generator{
    g, std::tie(genie_limiter), [&genie_counter](unsigned int const i) -> unsigned int {
      thread_counter c{genie_counter};
      spdlog::info("Generating {}", i);
      timed_busy(10ms);
      return i;
    }};

  serial_node propagator{g, tbb::flow::unlimited, [](unsigned int const i) -> unsigned int {
                           spdlog::info("Propagating {}", i);
                           timed_busy(250ms);
                           spdlog::info("Done propagating {}", i);
                           return i;
                         }};

  make_edge(src, histogrammer);
  make_edge(src, histo_generator);
  make_edge(src, generator);
  make_edge(src, propagator);

  root_limiter.activate();
  genie_limiter.activate();
  src.activate();

  g.wait_for_all();
}

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
  resource_limiter root_limiter{g, "ROOT", 1};

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

int main()
{
  serialize_functions_based_on_resource();
  serialize_functions_when_splitting_then_merging();
}
