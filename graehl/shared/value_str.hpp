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

    for configure.hpp library - print parsed values' original strings
*/

#ifndef VALUE_STR_JG2012613_HPP
#define VALUE_STR_JG2012613_HPP
#pragma once

#include <graehl/shared/string_to.hpp>
#include <graehl/shared/assign_traits.hpp>

namespace graehl {

struct value_str {
  boost::any value;
  std::string str;  // because we want to print without having the type around, and this is simpler than a
                    // boost::function

  value_str() {}
  value_str(value_str const& o) : value(o.value), str(o.str) {}
  template <class T>
  explicit value_str(T const& t) {
    *this = t;
  }
  template <class T>
  value_str(T const& t, std::string as_string)
      : value(t), str(as_string) {}

  template <class T>
  void assign_to(T& t) const  // must be same type as stored in value
  {
    assign_traits<T>::assign_any(t, value);
  }
  template <class T>
  T get() const {
    return boost::any_cast<T>(value);
  }
  template <class T>
  void operator=(T const& t) {
    value = t;
    str = to_string(t);
  }
  template <class O>
  void print(O& o) const {
    o << str;
  }
  template <class Ch, class Tr>
  friend std::basic_ostream<Ch, Tr>& operator<<(std::basic_ostream<Ch, Tr>& o, value_str const& self) {
    self.print(o);
    return o;
  }
};


}

#endif
