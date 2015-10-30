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

    o << x and i >> x for your type w/ x.read(i) and
    x.print(o), and helper printable x+context object if you require additional
    context e.g x.print(o, context)

    struct T {
    typedef T self_type;
    template <class charT, class Traits>
    void read(std::basic_istream<charT, Traits>& in)
    {
    }
    template <class charT, class Traits>
    void print(std::basic_ostream<charT, Traits>& o) const
    {
    }

   /// or, even shorter:

    template <class I>
    void read(I& in)
    {}

    template <class O>
    void print(O& o) const
    {}
};
*/

#ifndef GRAEHL__SHARED__PRINT_READ_HPP
#define GRAEHL__SHARED__PRINT_READ_HPP
#pragma once

#include <iostream>

#define TO_OSTREAM_PRINT                                                                                  \
  template <class Char, class CharTraits>                                                                 \
  friend inline std::basic_ostream<Char, CharTraits>& operator<<(std::basic_ostream<Char, CharTraits>& o, \
                                                                 self_type const& me) {                   \
    me.print(o);                                                                                          \
    return o;                                                                                             \
  }                                                                                                       \
  typedef self_type has_print;

#define FROM_ISTREAM_READ                                                                                 \
  template <class Char, class CharTraits>                                                                 \
  friend inline std::basic_istream<Char, CharTraits>& operator>>(std::basic_istream<Char, CharTraits>& i, \
                                                                 self_type& me) {                         \
    me.read(i);                                                                                           \
    return i;                                                                                             \
  }

#define TO_OSTREAM_PRINT_FREE(self_type)                                                    \
  template <class Char, class CharTraits>                                                   \
  std::basic_ostream<Char, CharTraits>& operator<<(std::basic_ostream<Char, CharTraits>& o, \
                                                   self_type const& me) {                   \
    me.print(o);                                                                            \
    return o;                                                                               \
  }

#define FROM_ISTREAM_READ_FREE(self_type)                                                                    \
  template <class Char, class CharTraits>                                                                    \
  std::basic_istream<Char, CharTraits>& operator>>(std::basic_istream<Char, CharTraits>& i, self_type& me) { \
    me.read(i);                                                                                              \
    return i;                                                                                                \
  }


#endif
