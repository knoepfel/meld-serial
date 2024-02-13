#include "serial/serializer_node.hpp"

namespace meld {
  serializers::serializers(tbb::flow::graph& g) : graph_{g} {}

  serializer_node& serializers::get(std::string const& name)
  {
    return serializers_.try_emplace(name, serializer_node{graph_, name}).first->second;
  }

  void serializers::activate()
  {
    for (auto& [name, serializer] : serializers_) {
      serializer.activate();
    }
  }
}
