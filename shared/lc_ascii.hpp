/** \file

    case-insensitive string keys
*/

#ifndef GRAEHL_SHARED__LC_ASCII_HPP
#define GRAEHL_SHARED__LC_ASCII_HPP
#pragma once


namespace graehl {

inline char lc_ascii(char c) {
  if (c >= 'A' && c <= 'Z') c -= ('A' - 'a');
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
