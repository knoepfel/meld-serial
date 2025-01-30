#ifndef meld_metaprogramming_type_deduction_hpp
#define meld_metaprogramming_type_deduction_hpp

#include "metaprogramming/detail/parameter_types.hpp"
#include "metaprogramming/detail/return_type.hpp"

#include <tuple>

namespace meld {
  template <typename T>
  using return_type = decltype(detail::return_type_impl(std::declval<T>()));

  template <typename T>
  using function_parameter_types = decltype(detail::parameter_types_impl(std::declval<T>()));

  template <std::size_t I, typename T>
  using function_parameter_type = std::tuple_element_t<I, function_parameter_types<T>>;
}

#endif // meld_metaprogramming_type_deduction_hpp
