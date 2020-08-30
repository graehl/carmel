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

   provide the slightly faster storage-owning:

    SprintfStr str("%d %f %s, 1, 1.f, "cstring")

    is the same as

    std::string str("1 1.0 cstring")

    if you need only a cstr, try

    SprintfCstr<MAX_EXPECTED_CHARS_PLUS_1> buf(...); // works even if your estimate is too low

    buf.c_str(); //only valid as long as buf object exists

*/

#ifndef SPRINTF_JG_2013_05_21_HPP
#define SPRINTF_JG_2013_05_21_HPP
#pragma once
#include <graehl/shared/cpp11.hpp>
#include <cassert>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string>

#ifndef GRAEHL_MUTABLE_STRING_DATA
/// see http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2668.htm
/// - &str[0] required to be a writable array just like std::vector
#define GRAEHL_MUTABLE_STRING_DATA 1
#endif

// gcc/clang have real va_copy if you use -std=c++0x or later
#define GRAEHL_va_free_copy(va) va_end(va)

namespace graehl {

/**
   usage: Sprintf<>("%d %f %s, 1, 1.f, "cstring").str() gives string("1 1.0
   cstring"). cstr (or char * implicit) is only valid as long as Sprintf object
   exists. implicitly converts to std::string

   default buflen=52 makes sizeof(Sprintf<>) == 64 (w/ 64-bit ptrs)
   - note that we'll still succeed even if 52 is too small (by heap alloc)
*/
/*

  That class is detected suspicious by cppcheck:
  <error id="va_list_usedBeforeStarted" severity="error" msg="va_list &apos;tmpva&apos; used before va_start() was called."
    verbose="va_list &apos;tmpva&apos; used before va_start() was called." cwe="664">
  but not used anymore anyway so commented out.

template <unsigned buflen = 52>
struct SprintfCstr {
  char buf[buflen];
  unsigned size;
  char* cstr;

  /// with optimization, str() should be more efficient than std::string(*this),
  /// which would have to find strlen(cstr) first
  std::string str() const { return std::string(cstr, size); }

  operator std::string() const { return str(); }

  char* c_str() const { return cstr; }

  typedef char* const_iterator;
  typedef char* iterator;
  typedef char value_type;
  iterator begin() const { return cstr; }
  iterator end() const { return cstr + size; }

  SprintfCstr() : size(), cstr() {}  // NOLINT

  void clear() {
    free();
    size = 0;
    cstr = 0;
  }

  void set(char const* format, va_list va) {
    clear();
    init(format, va);
  }

  void set(char const* format, ...) {  // NOLINT
    va_list va;
    va_start(va, format);  // NOLINT
    set(format, va);
    va_end(va);
  }

  SprintfCstr(char const* format, va_list va) { init(format, va); }  // NOLINT

  SprintfCstr(char const* format, ...) {  // NOLINT
    va_list va;
    va_start(va, format);  // NOLINT
    init(format, va);
    va_end(va);
  }

  ~SprintfCstr() { free(); }

  enum { first_buflen = buflen };

 protected:
  void init(char const* format, va_list va) {
    va_list tmpva;
    va_copy(tmpva, va);
    size = (unsigned)std::vsnprintf(buf, buflen, format, tmpva);
    GRAEHL_va_free_copy(tmpva);
    assert(size != (unsigned)-1);
    if (size >= buflen) {
      unsigned heapsz = size + 1;
      assert(heapsz);
      cstr = (char*)std::malloc(heapsz);
      size = std::vsnprintf(cstr, heapsz, format, va);
    } else
      cstr = buf;
    GRAEHL_va_free_copy(tmpva);
  }

  void free() {
    assert(size != (unsigned)-1);
    if (size >= buflen) std::free(cstr);
  }
};
*/

#if GRAEHL_MUTABLE_STRING_DATA
/**
   if you're going to SprintfCstr<>(...).str(), this would be faster as it avoids
   a copy. this implicitly converts to (is a) std::string
*/
template <unsigned buflen = 132>
struct Sprintf : std::string {

  /// with optimization, str() should be more efficient than std::string(*this),
  /// which would have to find strlen(cstr) first

  void set(char const* format, va_list va) { reinit(format, va); }

  void set(char const* format, ...) {  // NOLINT
    va_list va;
    va_start(va, format);  // NOLINT
    set(format, va);
    va_end(va);
  }

  Sprintf() {}

  Sprintf(char const* format, va_list va) : std::string(first_buflen, '\0') { init(format, va); }

  /**
     these overloads are better than a C varargs version because they avoid the
     (possibly non-optimizable va_list copy overhead). calling sprintf twice
     (once to get length, and then possibly again to allocate) requires copying
     varargs in GCC even if no reallocation is needed.
  */
  template <class V1>
  Sprintf(char const* format, V1 const& v1) : std::string(first_buflen, '\0') {
    unsigned size = (unsigned)std::snprintf(buf(), buflen, format, v1);  // NOLINT
    if (size >= first_buflen) {
      unsigned heapsz = size + 1;
      resize(heapsz);
      size = std::snprintf(buf(), heapsz, format, v1);  // NOLINT
    }
    resize(size);
  }

  template <class V1, class V2>
  Sprintf(char const* format, V1 const& v1, V2 const& v2) : std::string(first_buflen, '\0') {
    unsigned size = (unsigned)std::snprintf(buf(), buflen, format, v1, v2);  // NOLINT
    if (size >= first_buflen) {
      unsigned heapsz = size + 1;
      resize(heapsz);
      size = std::snprintf(buf(), heapsz, format, v1, v2);  // NOLINT
    }
    resize(size);
  }

  template <class V1, class V2, class V3>
  Sprintf(char const* format, V1 const& v1, V2 const& v2, V3 const& v3) : std::string(first_buflen, '\0') {
    unsigned size = (unsigned)std::snprintf(buf(), buflen, format, v1, v2, v3);  // NOLINT
    if (size >= first_buflen) {
      unsigned heapsz = size + 1;
      resize(heapsz);
      size = std::snprintf(buf(), heapsz, format, v1, v2, v3);  // NOLINT
    }
    resize(size);
  }

  Sprintf(char const* format, ...) : std::string(first_buflen, '\0') {  // NOLINT
    va_list va;
    va_start(va, format);  // NOLINT
    init(format, va);
    va_end(va);
  }
  enum { first_buflen = buflen };

 protected:
  void reinit(char const* format, va_list va) {
    if (size() < first_buflen) resize(first_buflen);
    init(format, va);
  }

  char* buf() { return &(*this)[0]; }

  void init(char const* format, va_list va) {
    va_list tmpva;
    va_copy(tmpva, va);
    unsigned size = (unsigned)std::vsnprintf(buf(), buflen, format, tmpva);  // NOLINT
    GRAEHL_va_free_copy(tmpva);
    assert(size != (unsigned)-1);
    if (size >= first_buflen) {
      unsigned heapsz = size + 1;
      assert(heapsz);
      resize(heapsz);
      size = std::vsnprintf(buf(), heapsz, format, va);
    }
    resize(size);
  }
};

#else  // GRAEHL_MUTABLE_STRING_DATA
#define Sprintf SprintfCstr
#endif

typedef Sprintf<> SprintfStr;


}

#endif
