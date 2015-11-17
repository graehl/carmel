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
// broken attempt to supply default oeprator << if A::default_print
#ifndef DEFAULT_PRINT_ON_HPP
#define DEFAULT_PRINT_ON_HPP
#include <iostream>

#include <vector>

namespace graehl {


template <class V, class charT, class Traits>
std::basic_ostream<charT, Traits> & operator << (std::basic_ostream<charT, Traits> &o, const std::vector<V> &v)
{
  o << '[';
  bool first = true;
  for (typename std::vector<V>::const_iterator i = v.begin(), e = v.end(); i!=e; ++i) {
    if (first)
      first = false;
    else
      o << ',';
    o << *i;
  }
  o << ']';
  return o;
}


//# include "print.hpp"

/*
  template <class C,class V=void>
  struct default_print;

  template <class C,class V>
  struct default_print {
  };

  template <class C>
  struct default_print<C,typename C::default_print> {
  typedef void type;
  };
*/

//USAGE: typedef Self default_print
//FIXME: doesn't work
template <class A, class charT, class Traits>
inline std::basic_ostream<charT, Traits>&
operator << (std::basic_ostream<charT, Traits>& s, const typename A::default_print &arg)
{
  if (!s.good()) return s;
  std::ios_base::iostate err = std::ios_base::goodbit;
  typename std::basic_ostream<charT, Traits>::sentry sentry(s);
  if (sentry)
    err = arg.print(s);
  if (err)
    s.setstate(err);
  return s;
}

}
#endif
