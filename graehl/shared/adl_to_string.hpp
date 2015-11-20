// Copyright 2014 Jonathan Graehl-http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/** \file

    call (ADL) string_to_impl(v) in v's namespace.
*/

#ifndef ADL_TO_STRING_GRAEHL_2015_10_29_HPP
#define ADL_TO_STRING_GRAEHL_2015_10_29_HPP
#pragma once

#include <string>
#include <graehl/shared/string_to.hpp>

namespace graehl {}

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
