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

    case-insensitive string keys
*/

#ifndef GRAEHL_SHARED__LC_ASCII_HPP
#define GRAEHL_SHARED__LC_ASCII_HPP
#pragma once


namespace graehl {

inline char lc_ascii(char c) {
  if (c >= 'A' && c <= 'Z') c -= ('A'-'a');
  return c;
}

template <class String>
String& lc_ascii_inplace(String& s) {
  for (typename String::iterator i = s.begin(), e = s.end(); i != e; ++i) *i = lc_ascii(*i);
  return s;
}

template <class String>
void append_lc_ascii(String& r, char const* s) {
  while (*s) r.push_back(lc_ascii(*s++));
}

template <class String>
void set_lc_ascii(String& r, char const* s) {
  r.clear();
  append_lc_ascii(r, s);
}


}

#endif
