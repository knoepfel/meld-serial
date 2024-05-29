#include "oneapi/tbb/flow_graph.h"
#include "spdlog/spdlog.h"

#include <memory>
#include <string>

using namespace oneapi::tbb;

using intp = std::shared_ptr<int>;

namespace {
  auto printer_for = [](tbb::flow::graph& g, std::string label) {
    return flow::function_node<intp>{g, flow::unlimited, [name = std::move(label)](intp const& p) {
                                       if (p) {
                                         spdlog::info("{}: {}", name, *p);
                                       }
                                       else {
                                         spdlog::info("{}: empty", name);
                                       }
                                       return flow::continue_msg{};
                                     }};
  };

  template <typename T>
  class short_circuiter : public flow::function_node<std::shared_ptr<T>, std::shared_ptr<T>> {
    using base_t = flow::function_node<std::shared_ptr<T>, std::shared_ptr<T>>;

  public:
    template <typename FT>
    short_circuiter(flow::graph& g, std::size_t concurrency, FT f) :
      base_t{g, concurrency, [func = std::move(f)](std::shared_ptr<T> p) -> std::shared_ptr<T> {
               if (p) {
                 return std::make_shared<T>(func(*p));
               }
               return nullptr;
             }}
    {
    }
  };
}

int main()
{

  flow::graph g;
  int i{};
  flow::input_node src{g, [&i](flow_control& fc) -> intp {
                         if (i == 10) {
                           fc.stop();
                           return {};
                         }

                         auto j = i++;
                         if (j % 2 == 0) {
                           return std::make_shared<int>(j);
                         }
                         return nullptr;
                       }};

  auto pointer_printer = printer_for(g, "squared pointer printer");

  auto user_func = [](int const& n) { return n * n; };
  short_circuiter<int> squarer{g, flow::serial, user_func};

  make_edge(src, squarer);
  make_edge(squarer, pointer_printer);

  src.activate();
  g.wait_for_all();
}
