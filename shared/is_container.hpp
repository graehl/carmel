// Copyright 2014 Jonathan Graehl - http://graehl.org/
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

    boost type_traits for 'is a container' (for printing containers, for
    e.g. show.hpp debug prints) possibly excluding std::string (which you
    usually want to print normally). does not exclude char* or other
    string-likes.
*/


#ifndef IS_CONTAINER_JG2012614_HPP
#define IS_CONTAINER_JG2012614_HPP
#pragma once

#include <graehl/shared/type_traits.hpp>

namespace graehl {

template <class T, class Enable = void>
struct is_nonstring_container : is_container<T> {};

template <class charT, class Traits>
struct is_nonstring_container<std::basic_string<charT, Traits>, void> {
  enum { value = 0 };
};


template <class Val, class Enable = void>
struct print_maybe_container {
  template <class O>
  void print(O& o, Val const& val, bool bracket = false) {
    o << val;
  }
};

template <class Val>
struct print_maybe_container<Val, typename enable_if<is_nonstring_container<Val>::value>::type> {
  template <class O>
  void print(O& o, Val const& val, bool bracket = false) {
    bool first = true;
    if (bracket) o << '[';
    for (typename Val::const_iterator i = val.begin(), e = val.end(); i != e; ++i) {
      if (!first) o << ' ';
      o << *i;
      first = false;
    }
    if (bracket) o << ']';
  }
};

#if GRAEHL_CPP11
template <class T>
using is_container_t = typename is_container<T>::type;
template <class T>
using is_nonstring_container_t = typename is_nonstring_container<T>::type;
#endif

}

#endif
