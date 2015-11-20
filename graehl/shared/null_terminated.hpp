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

    write algorithms that work on std::string and c-string alike (template on
    string-end convention)

usage:

   char const* s="ab";

   cstr_const_iterator(s), cstr_const_iterator()

   or

   null_terminated_begin(s), null_terminated_end(s)
*/

#ifndef GRAEHL__SHARED__NULL_TERMINATED_HPP
#define GRAEHL__SHARED__NULL_TERMINATED_HPP
#pragma once

#include <boost/iterator/iterator_facade.hpp>
#include <graehl/shared/is_null.hpp>
#include <cstring>
#include <iterator>

namespace graehl {


template <class C>
class null_terminated_iterator
    : public boost::iterator_facade<null_terminated_iterator<C>, C, boost::forward_traversal_tag> {
 public:
  // end is default constructed
  null_terminated_iterator(C* p = 0) : p(p) {}

  static inline null_terminated_iterator<C> end() { return null_terminated_iterator<C>(); }

 private:
  C* p;

  friend class boost::iterator_core_access;

  void increment() {
    if (is_null(*++p)) p = 0;
  }
  /* //note: you can't decrement from end==default constructed because it's 0, not address of null
      void decrement()
      {
        --p;
      }
  */
  C& dereference() const { return *p; }

  bool equal(null_terminated_iterator<C> const& other) const { return p == other.p; }
};

typedef null_terminated_iterator<char> cstr_iterator;
typedef null_terminated_iterator<char const> const_cstr_iterator;
typedef null_terminated_iterator<char const> cstr_const_iterator;

inline char const* null_terminated_end(char const* s) {
  return s + std::strlen(s);
}

template <class C>
inline C const* null_terminated_end(C const* s) {
  while (*s++)
    ;
  return s;
}


/*
inline std::reverse_iterator<char const*> null_terminated_rbegin(char const* s)
{
  return std::reverse_iterator<char const*>(null_terminated_end(s));
}

inline std::reverse_iterator<char const*> null_terminated_rend(char const* s)
{
  return std::reverse_iterator<char const*>(s);
}

template <class C>
inline std::reverse_iterator<C const*> null_terminated_rbegin(C const* s)
{
  return null_terminated_end(s);
}

template <class C>
inline std::reverse_iterator<C const*> null_terminated_rend(C const* s)
{
  return s;
}
*/

// nonconst (copies of above)
inline char* null_terminated_end(char* s) {
  return s + std::strlen(s);
}

template <class C>
inline C* null_terminated_end(C* s) {
  while (*s++)
    ;
  return s;
}


}

#endif
