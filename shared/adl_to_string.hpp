/** \file

    call (ADL) string_to_impl(v) in v's namespace.
*/

#ifndef ADL_TO_STRING_GRAEHL_2015_10_29_HPP
#define ADL_TO_STRING_GRAEHL_2015_10_29_HPP
#pragma once

#include <string>
#include <graehl/shared/string_to.hpp>

namespace graehl {
}

namespace adl {

template <class V>
inline void adl_to_string(V const&);

template <class V, class Enable = void>
struct ToString {
  template <class O>
  static std::string call(V const& v) {
    using namespace graehl;
    return string_to_impl(v);
  }
};

template <>
struct ToString<std::string, void> {
  template <class O>
  static std::string const& call(std::string const& v) {
    return v;
  }
};

template <>
struct ToString<std::string, void> {
  template <class O>
  static std::string call(char const* v) {
    return v;
  }
};

}

#endif
