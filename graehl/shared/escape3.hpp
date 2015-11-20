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
/**
   print \0a instead of newline and other nonprintable ascii chars and
   backslash. inspired by mdb_dump format (lightning mdb)
*/

#ifndef GRAEHL_SHARED__ESCAPE3
#define GRAEHL_SHARED__ESCAPE3
#pragma once

#include <cassert>
#include <string>
#include <graehl/shared/string_to.hpp>
#include <graehl/shared/atoi_fast.hpp>

namespace graehl {

static char const* const k_hex = "0123456789ABCDEF";

inline bool plaintext_escape3(char c) {
  return c >= 32 && c < 127 && c != '\\';
}

inline void append_escape3(char*& out, unsigned char c) {
  if (plaintext_escape3(c))
    *out++ = c;
  else {
    *out++ = '\\';
    *out++ = k_hex[c >> 4];
    *out++ = k_hex[c & 0xf];
  }
}

inline std::size_t reserve_escape3(std::size_t len) {
  return len * 3;
}

/// return true if truncated (can print ... or something). append to empty s.
template <class String>
inline bool escape3(char const* i, std::size_t n, String& s, std::size_t maxlen = 0) {
  assert(s.empty());
  if (!maxlen || maxlen > n) maxlen = n;
  if (!maxlen) return false;
  s.resize(reserve_escape3(maxlen));
  char* out = &s[0];
  char* obegin = out;
  char const* end = i + maxlen;
  for (; i < end; ++i) append_escape3(out, *i);
  s.resize(out - obegin);
  return maxlen < n;
}

template <class String>
inline bool escape3(void const* i, std::size_t n, String& s, std::size_t maxlen = 0) {
  return escape3((char const*)i, n, s, maxlen);
}

/// this is ostream-printable e.g. cout << Escape3(str)
struct Escape3 : string_builder {
  Escape3(void const* i, std::size_t n, std::size_t maxlen = 0, bool nbytes = false) { append(i, n, maxlen); }
  Escape3(std::size_t n, void const* i, std::size_t maxlen = 0, bool nbytes = false) { append(i, n, maxlen); }
  explicit Escape3(std::string const& str, std::size_t maxlen = 0, bool nbytes = false) {
    append(str.data(), str.size(), maxlen);
  }
  void append(void const* i, std::size_t n, std::size_t maxlen = 0, bool nbytes = false) {
    if (escape3(i, n, *this, maxlen)) {
      push_back('.');
      push_back('.');
      push_back('.');
    }
    if (nbytes) {
      push_back(' ');
      push_back('(');
      operator()(n);
      operator()(" bytes)");
    }
  }
};

struct Escape3Exception : std::exception {
  Escape3Exception() {}
  ~Escape3Exception() throw() {}
  const char* what() const throw() {
    return "Unescape3 expected two hex digits or backslash following backslash";
  }
};

template <class ByteIt, class String>
void unescape3(ByteIt i, ByteIt e, String& s) {
  for (; i != e; ++i) {
    char c = *i;
    if (c != '\\')
      s.push_back(c);
    else {
      if (++i == e) throw Escape3Exception();
      c = *i;
      if (c == '\\')
        s.push_back(c);
      else {
        unsigned char high = hexdigit(c);
        if (++i == e) throw Escape3Exception();
        unsigned char low = hexdigit(*i);
        s.push_back(high * 16 + low);
      }
    }
  }
}

template <class String>
void unescape3(void const* i, std::size_t n, String& s) {
  unescape3((char const*)i, (char const*)i + n, s);
}

template <class String>
void unescape3(char const* in, String& s) {
  unescape3(in, in + strlen(in), s);
}

template <class String>
void unescape3(std::string const& in, String& s) {
  unescape3(in.begin(), in.end(), s);
}


}

#endif
