#include "oneapi/tbb/flow_graph.h"
#include "short_circuiter/short_circuiter_node.hpp"
#include "short_circuiter/simple_short_circuiter_node.hpp"
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
          spdlog::info("{}: {}", name, *m.data);
        }
        else {
          spdlog::info("{}: empty", name);
        }
        return flow::continue_msg{};
      }};
  }
}

int main()
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

  auto printer = printer_for<int>(g, "printer for squarer");
  auto simple_printer = printer_for<int>(g, "printer for simple_squarer");

  auto user_func = [](int const& n) { return n * n; };
  short_circuiter<std::tuple<int>, int> squarer{g, flow::serial, user_func};
  simple_short_circuiter<std::tuple<int>, int> simple_squarer{g, flow::serial, user_func};

  make_edge(src, squarer);
  make_edge(src, simple_squarer);
  make_edge(squarer, printer);
  make_edge(simple_squarer, simple_printer);

  src.activate();
  g.wait_for_all();
}
