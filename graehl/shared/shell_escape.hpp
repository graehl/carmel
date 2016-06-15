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

    round-trip unix-shell command line strings.
*/

#ifndef GRAEHL_SHARED__SHELL_ESCAPE_HPP
#define GRAEHL_SHARED__SHELL_ESCAPE_HPP
#pragma once

#include <graehl/shared/string_to.hpp>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace graehl {

template <class C>
inline bool is_shell_special(C c) {
  switch (c) {
    case ' ':
    case '\t':
    case '\n':
    case '\\':
    case '>':
    case '<':
    case '|':
    case '&':
    case ';':
    case '"':
    case '\'':
    case '`':
    case '~':
    case '*':
    case '?':
    case '{':
    case '}':
    case '$':
    case '!':
    case '(':
    case ')': return true;
    default: return false;
  }
}

template <class C>
inline bool needs_shell_escape_in_quotes(C c) {
  switch (c) {
    case '\\':
    case '"':
    case '$':
    case '`':
    case '!': return true;
    default: return false;
  }
}


// TODO: istreambuf_iterator probably slower than conversion to string

template <class Ch, class Tr>
struct out_stream {
  std::basic_ostream<Ch, Tr>& o;
  out_stream(std::basic_ostream<Ch, Tr>& o) : o(o) {}
  out_stream(out_stream const& o) : o(o.o) {}
  void operator()(char c) const { o.put(c); }
  template <class T>
  void operator()(T const& t) const {
    o << t;
  }
};


template <class CharAcceptor, class CharForwardIter>
inline CharAcceptor const& shell_quote_chars_iter(CharAcceptor const& chars, CharForwardIter i,
                                                  CharForwardIter end, bool quote_empty = true) {
  if (i == end) {
    if (quote_empty) {
      chars('"');
      chars('"');
    }
  } else if (std::find_if(i, end, is_shell_special<char>) == end) {
    for (; i != end; ++i) chars(*i);
  } else {
    chars('"');
    for (; i != end; ++i) {
      char c = *i;
      if (needs_shell_escape_in_quotes(c)) chars('\\');
      chars(c);
    }
    chars('"');
  }
  return chars;
}

template <class CharAcceptor, class Str>
inline CharAcceptor const& shell_quote_chars(CharAcceptor const& chars, Str const& str, bool quote_empty = true) {
  return shell_quote_chars_iter(chars, str.begin(), str.end(), quote_empty);
}

template <class C, class Ch, class Tr>
inline std::basic_ostream<Ch, Tr>& out_shell_quote(std::basic_ostream<Ch, Tr>& out, C const& data,
                                                   bool quote_empty = true) {
  out_stream<Ch, Tr> chars(out);
  shell_quote_chars(chars, to_string(data), quote_empty);
  return out;
}

// function object pointing to string_builder or buffer. cheap copy
struct append_string_buffer {
  string_buffer& b;
  append_string_buffer(string_buffer& b) : b(b) { b.reserve(100); }
  void operator()(char c) const { b.push_back(c); }
};

template <class C>
inline std::string shell_quote(C const& data, bool quote_empty = true) {
  string_buffer b;  // NRVO in C++11
  shell_quote_chars(append_string_buffer(b), to_string(data), quote_empty);
  return strcopy(b);
}


}

#endif
