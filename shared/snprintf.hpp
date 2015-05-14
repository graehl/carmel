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
/**

   provide ::snprintf for MSVC, which lacks the C99/posix/C++11 snprintf, and
   ::C99vsnprintf (varargs version)

   also, the slightly faster storage-owning:

    SprintfStr str("%d %f %s, 1, 1.f, "cstring")

    is the same as

    std::string str("1 1.0 cstring")

    if you need only a cstr, try

    SprintfCstr<MAX_EXPECTED_CHARS_PLUS_1> buf(...); // works even if your estimate is too low

    buf.c_str(); //only valid as long as buf object exists

*/

#ifndef SPRINTF_JG_2013_05_21_HPP
#define SPRINTF_JG_2013_05_21_HPP
#include <cstdio>
#include <cstddef>
#include <cstdarg>
#include <cassert>
#include <cstdlib>
#pragma once
// for MS, this has some microsoft-only _snprintf etc fns that aren't fully C99 compliant - we'll provide a
// ::snprintf that is
#include <string>

#ifndef GRAEHL_MUTABLE_STRING_DATA
#if __cplusplus >= 201103L
/// see http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2668.htm
/// - &str[0] required to be a writable array just like std::vector
#define GRAEHL_MUTABLE_STRING_DATA 1
#else
#ifdef _MSC_VER
#if _MSC_VER >= 1500
// VS 2008 or later
#define GRAEHL_MUTABLE_STRING_DATA 1
#else
#define GRAEHL_MUTABLE_STRING_DATA 0
#endif
#else
#define GRAEHL_MUTABLE_STRING_DATA 1
#endif
#endif
#endif

#ifdef _MSC_VER
#define GRAEHL_va_free_copy(va)
// C++11/C99 requires va_copy - msvc doesn't have one
#ifndef va_copy
#define va_copy(dst, src) ((dst) = (src))
#endif
#else
// gcc/clang have real va_copy
#define GRAEHL_va_free_copy(va) va_end(va)
#endif

// in unix we already have a C99 compliant ::snprintf
#if defined(_MSC_VER) && !defined(CRT_NO_DEPRECATE)

#ifndef snprintf
#define snprintf C99snprintf
#endif

/**
   conforms to C99 vsnprintf - you can call w/ buflen less than required, and
   return is number of chars that would have been written (so buflen should be >
   than that by at least 1 for '\0'
*/
inline int C99vsnprintf(char* buf, std::size_t buflen, const char* format, va_list va) {
  if (buflen) {
    va_list tmpva;
    va_copy(tmpva, va);
    // unfortunately, windows *snprintf_s return -1 if buffer was too small.
    int count = _vsnprintf_s(buf, buflen, _TRUNCATE, format, tmpva);
    GRAEHL_va_free_copy(tmpva);
    if (count >= 0) return count;
  }
  return _vscprintf(format, va);  // counts # of chars that would be written
}

/**
   conforms to C99 snprintf - you can call w/ buflen less than required, and
   return is number of chars that would have been written (so buflen should be >
   than that by at least 1 for '\0'
*/
inline int C99snprintf(char* buf, std::size_t buflen, const char* format, ...) {
  va_list va;
  va_start(va, format);
  int count = C99vsnprintf(buf, buflen, format, va);
  va_end(va);
  return count;
}
// end MSVC C99 wrappers
#else
// unix:
using std::snprintf;
#define C99vsnprintf std::vsnprintf
#define C99snprintf std::snprintf
#endif

namespace graehl {

/**
   usage: Sprintf<>("%d %f %s, 1, 1.f, "cstring").str() gives string("1 1.0
   cstring"). cstr (or char * implicit) is only valid as long as Sprintf object
   exists. implicitly converts to std::string

   default buflen=52 makes sizeof(Sprintf<>) == 64 (w/ 64-bit ptrs)
   - note that we'll still succeed even if 52 is too small (by heap alloc)
*/
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

  SprintfCstr() : size(), cstr() {}

  void clear() {
    free();
    size = 0;
    cstr = 0;
  }

  void set(char const* format, va_list va) {
    clear();
    init(format, va);
  }

  void set(char const* format, ...) {
    va_list va;
    va_start(va, format);
    set(format, va);
    va_end(va);
  }

  SprintfCstr(char const* format, va_list va) { init(format, va); }

  SprintfCstr(char const* format, ...) {
    va_list va;
    va_start(va, format);
    init(format, va);
    va_end(va);
  }

  ~SprintfCstr() { free(); }

  enum { first_buflen = buflen };

 protected:
  void init(char const* format, va_list va) {
    va_list tmpva;
    va_copy(tmpva, va);
    size = (unsigned)C99vsnprintf(buf, buflen, format, tmpva);
    GRAEHL_va_free_copy(tmpva);
    assert(size != (unsigned)-1);
    if (size >= buflen) {
      unsigned heapsz = size + 1;
      assert(heapsz);
      cstr = (char*)std::malloc(heapsz);
      size = C99vsnprintf(cstr, heapsz, format, va);
    } else
      cstr = buf;
    GRAEHL_va_free_copy(tmpva);
  }

  void free() {
    assert(size != (unsigned)-1);
    if (size >= buflen) std::free(cstr);
  }
};


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

  void set(char const* format, ...) {
    va_list va;
    va_start(va, format);
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
  Sprintf(char const* format, V1 const& v1)
      : std::string(first_buflen, '\0') {
    unsigned size = (unsigned)C99snprintf(buf(), buflen, format, v1);
    if (size >= first_buflen) {
      unsigned heapsz = size + 1;
      resize(heapsz);
      size = C99snprintf(buf(), heapsz, format, v1);
    }
    resize(size);
  }

  template <class V1, class V2>
  Sprintf(char const* format, V1 const& v1, V2 const& v2)
      : std::string(first_buflen, '\0') {
    unsigned size = (unsigned)C99snprintf(buf(), buflen, format, v1, v2);
    if (size >= first_buflen) {
      unsigned heapsz = size + 1;
      resize(heapsz);
      size = C99snprintf(buf(), heapsz, format, v1, v2);
    }
    resize(size);
  }

  template <class V1, class V2, class V3>
  Sprintf(char const* format, V1 const& v1, V2 const& v2, V3 const& v3)
      : std::string(first_buflen, '\0') {
    unsigned size = (unsigned)C99snprintf(buf(), buflen, format, v1, v2, v3);
    if (size >= first_buflen) {
      unsigned heapsz = size + 1;
      resize(heapsz);
      size = C99snprintf(buf(), heapsz, format, v1, v2, v3);
    }
    resize(size);
  }

  Sprintf(char const* format, ...) : std::string(first_buflen, '\0') {
    va_list va;
    va_start(va, format);
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
    unsigned size = (unsigned)C99vsnprintf(buf(), buflen, format, tmpva);
    GRAEHL_va_free_copy(tmpva);
    assert(size != (unsigned)-1);
    if (size >= first_buflen) {
      unsigned heapsz = size + 1;
      assert(heapsz);
      resize(heapsz);
      size = C99vsnprintf(buf(), heapsz, format, va);
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
