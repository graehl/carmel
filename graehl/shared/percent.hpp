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

    print percentages.
*/

#ifndef GRAEHL__SHARED__PERCENT_HPP
#define GRAEHL__SHARED__PERCENT_HPP
#pragma once

#include <graehl/shared/print_width.hpp>
#include <iosfwd>

namespace graehl {

template <int width = 5>
struct percent {
  double frac;
  percent(double f) : frac(f) {}
  percent(double num, double den) : frac(den ? num / den : 0) {}
  double get_percent() const { return frac * 100; }
  template <class C, class T>
  void print(std::basic_ostream<C, T>& o) const {
    print_max_width_small(o, get_percent(), width - 1);
    o << '%';
  }
  typedef percent<width> self_type;
};

template <class C, class T, int W>
std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& os, percent<W> const& p) {
  p.print(os);
  return os;
}

template <int width = 5>
struct portion {
  double num, den;

  portion(double num, double den) : num(num), den(den) {}

  double get_fraction() const { return (double)num / den; }

  template <class C, class T>
  void print(std::basic_ostream<C, T>& o) const {
    o << percent<width>(num, den);
    o << " (" << num << "/" << den << ")";
  }
};

template <class C, class T, int W>
std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& os, portion<W> const& p) {
  p.print(os);
  return os;
}


}

#endif
