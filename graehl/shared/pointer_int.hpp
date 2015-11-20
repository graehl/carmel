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

   for aligned pointer to objects of size>1, store integers x as 2*x + 1 (with lsb set).

*/

#ifndef GRAEHL_SHARED__POINTER_INT_JG_2013_06_10_HPP
#define GRAEHL_SHARED__POINTER_INT_JG_2013_06_10_HPP
#pragma once

#include <cstddef>  //size_t
#include <cassert>
#include <iomanip>

namespace graehl {

static const std::size_t pointer_int_max = (std::size_t)-1 >> 1;

/**
   for all but char * (or same sized, alignment means there's no pointer w/ lsb 1.
*/
inline bool is_pointer(void* p) {
  return !((std::size_t)p & 1);
}

inline bool is_pointer_int(void* p) {
  return (std::size_t)p & 1;
}

inline std::size_t pointer_int(void* p) {
  assert(is_pointer_int(p));
  return (std::size_t)p >> 1;
}

inline void* int_pointer(std::size_t n) {
  assert(n <= pointer_int_max);
  return (void*)(n << 1 | 1);
}

template <class Val>
inline void set_pointer_int(Val*& p, std::size_t n) {
  assert(n <= pointer_int_max);
  p = (Val*)(n << 1 | 1);
}

struct PointerInt {
  void* p;
  template <class Val>
  PointerInt(Val* p)
      : p(p) {}
  friend inline std::ostream& operator<<(std::ostream& out, PointerInt const& self) {
    self.print(out);
    return out;
  }
  void print(std::ostream& out) const {
    if (is_pointer_int(p))
      out << pointer_int(p);
    else
      out << "0x" << std::hex << (std::size_t)p << std::dec;
  }
};


}

#endif
