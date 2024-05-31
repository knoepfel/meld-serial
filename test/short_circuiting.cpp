#include "oneapi/tbb/flow_graph.h"
#include "short_circuiter/short_circuiter_node.hpp"
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

  auto pointer_printer = printer_for<int>(g, "squared pointer printer");

  auto user_func = [](int const& n) { return n * n; };
  short_circuiter<std::tuple<int>, int> squarer{g, flow::serial, user_func};

  make_edge(src, squarer);
  make_edge(squarer, pointer_printer);

  src.activate();
  g.wait_for_all();
}
