#include "serial/serial_node.hpp"
#include "serial/thread_counter.hpp"
#include "serial/timed_busy.hpp"

#include "oneapi/tbb/flow_graph.h"
#include "spdlog/spdlog.h"

#include <atomic>
#include <iostream>
#include <string>
#include <string_view>

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

void start(std::string_view algorithm, unsigned int spill, unsigned int data = 0u)
{
  spdlog::info("Start\t{}\t{}\t{}", algorithm, spill, data);
}

void stop(std::string_view algorithm, unsigned int spill, unsigned int data = 0u)
{
  spdlog::info("Stop\t{}\t{}\t{}", algorithm, spill, data);
}

void serialize_functions_based_on_resource()
{
  std::cout << "time\tthread\tevent\tnode\tmessage\tdata\n";
  spdlog::set_pattern("%H:%M:%S.%f\t%t\t%v");
  flow::graph g;
  unsigned int i{};
  flow::input_node src{g, [&i](flow_control& fc) {
                         if (i < 50) {
                           start("Source", i + 1); // The message is the spill id we will emit
                           auto j = ++i;
                           stop("Source", i);
                           return j;
                         }
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
    start("Histogramming", i);
    timed_busy(10ms);
    stop("Histogramming", i);
    return i;
  };

  auto gen_fill_histo = [&root_counter, &genie_counter](unsigned int const i) -> unsigned int {
    thread_counter c1{root_counter};
    thread_counter c2{genie_counter};
    start("Histo-generating", i);
    timed_busy(10ms);
    stop("Histo-generating", i);
    return i;
  };

  auto generate = [&genie_counter](unsigned int const i) -> unsigned int {
    thread_counter c{genie_counter};
    start("Generating", i);
    timed_busy(10ms);
    stop("Generating", i);
    return i;
  };

  auto propagate = [](unsigned int const i) -> unsigned int {
    start("Propagating", i);
    timed_busy(150ms);
    stop("Propagating", i);
    return i;
  };

  serial_node histogrammer{g, std::tie(root_limiter), fill_histo};
  serial_node histo_generator{g, std::tie(root_limiter, genie_limiter), gen_fill_histo};
  serial_node generator{g, std::tie(genie_limiter), generate};
  serial_node propagator{g, tbb::flow::unlimited, propagate};

  // Nodes that use the DB resource limited to 2 tokens
  auto make_calibrator = [&db_counter](std::string_view algorithm) {
    return [&db_counter, algorithm](unsigned int const i, DB const* db) -> unsigned int {
      thread_counter c{db_counter, 2};
      start(algorithm, i, db->id);
      timed_busy(10ms);
      stop(algorithm, i, db->id);
      return i;
    };
  };

  serial_node calibratorA{g, std::tie(db_limiter), make_calibrator("Calibration[A]")};
  serial_node calibratorB{g, std::tie(db_limiter), make_calibrator("Calibration[B]")};
  serial_node calibratorC{g, std::tie(db_limiter), make_calibrator("Calibration[C]")};

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
