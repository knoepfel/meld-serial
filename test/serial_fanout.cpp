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

// ROOT is the representation of the ROOT resource.
struct ROOT {};

// GENIE is the representation of the GENIE resource.
struct GENIE {};

// DB is the representation of the DB resource.
// The value id is the database ID.
struct DB {
  unsigned int id;
};

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

  // Declare the counters that we use to verify that the resource constraints
  // are being met.
  std::atomic<unsigned int> root_counter{};
  std::atomic<unsigned int> genie_counter{};
  std::atomic<unsigned int> db_counter{};

  resource_limiter<ROOT> root_limiter{g, 1};
  resource_limiter<GENIE> genie_limiter{g, 1};
  // We can use a temporary vector to create the DB objects that will
  // be owned by db_limiter.
  resource_limiter<DB> db_limiter{g, {DB{1}, DB{13}}};

  auto fill_histo = [&root_counter](unsigned int const i) -> unsigned int {
    thread_counter c{root_counter};
    spdlog::info("Histogramming {}", i);
    timed_busy(10ms);
    return i;
  };

  auto gen_fill_histo = [&root_counter, &genie_counter](unsigned int const i) -> unsigned int {
    thread_counter c1{root_counter};
    thread_counter c2{genie_counter};
    spdlog::info("Histo-generating {}", i);
    timed_busy(10ms);
    spdlog::info("Done histo-generating {}", i);
    return i;
  };

  auto generate = [&genie_counter](unsigned int const i) -> unsigned int {
    thread_counter c{genie_counter};
    spdlog::info("Generating {}", i);
    timed_busy(10ms);
    return i;
  };

  auto propagate = [](unsigned int const i) -> unsigned int {
    spdlog::info("Propagating {}", i);
    timed_busy(150ms);
    spdlog::info("Done propagating {}", i);
    return i;
  };

  serial_node histogrammer{g, std::tie(root_limiter), fill_histo};
  serial_node histo_generator{g, std::tie(root_limiter, genie_limiter), gen_fill_histo};
  serial_node generator{g, std::tie(genie_limiter), generate};
  serial_node propagator{g, tbb::flow::unlimited, propagate};

  // Nodes that use the DB resource limited to 2 tokens
  // TODO: figure out how we pass the token into the user's function, which need to be
  // able to access the stateful resource handle.
  serial_node calibratorA{
    g, std::tie(db_limiter), [&db_counter](unsigned int const i) -> unsigned int {
      thread_counter c{db_counter, 2};
      spdlog::info("Calibrating[A] {}", i);
      timed_busy(10ms);
      return i;
    }};

  serial_node calibratorB{
    g, std::tie(db_limiter), [&db_counter](unsigned int const i) -> unsigned int {
      thread_counter c{db_counter, 2};
      spdlog::info("Calibrating[B] {}", i);
      timed_busy(10ms);
      return i;
    }};

  serial_node calibratorC{
    g, std::tie(db_limiter), [&db_counter](unsigned int const i) -> unsigned int {
      thread_counter c{db_counter, 2};
      spdlog::info("Calibrating[C] {}", i);
      timed_busy(10ms);
      return i;
    }};

  make_edge(src, histogrammer);
  make_edge(src, histo_generator);
  make_edge(src, generator);
  make_edge(src, propagator);
  make_edge(src, calibratorA);
  make_edge(src, calibratorB);
  make_edge(src, calibratorC);

  root_limiter.activate();
  genie_limiter.activate();
  db_limiter.activate();
  src.activate();

  g.wait_for_all();
}

int main() { serialize_functions_based_on_resource(); }
