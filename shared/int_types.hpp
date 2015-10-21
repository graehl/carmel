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

    for templates: jump between unsigned and signed int types of same width
*/

#ifndef INT_TYPES_JG2012531_HPP
#define INT_TYPES_JG2012531_HPP
#pragma once

#if defined(__APPLE__) && defined(__GNUC__)
#define GRAEHL_INT_DIFFERENT_FROM_INTN 0
#define GRAEHL_PTRDIFF_DIFFERENT_FROM_INTN 0
#endif

#if !__clang__ && (__GNUC__ == 4 && __GNUC__ == 4 && __GNUC__MINOR >= 8)
#define GRAEHL_INT_DIFFERENT_FROM_INTN 1
#define GRAEHL_PTRDIFF_DIFFERENT_FROM_INTN 1
#endif

#ifndef GRAEHL_INT_DIFFERENT_FROM_INTN
#define GRAEHL_INT_DIFFERENT_FROM_INTN 0
#endif

#ifndef GRAEHL_PTRDIFF_DIFFERENT_FROM_INTN
#define GRAEHL_PTRDIFF_DIFFERENT_FROM_INTN 0
#endif

#ifndef GRAEHL_HAVE_LONGER_LONG
#define GRAEHL_HAVE_LONGER_LONG 0
#endif

#ifndef GRAEHL_HAVE_LONG_DOUBLE
#define GRAEHL_HAVE_LONG_DOUBLE 0
#endif

#ifndef GRAEHL_HAVE_64BIT_INT64_T
#define GRAEHL_HAVE_64BIT_INT64_T 1
#endif

#include <boost/cstdint.hpp>
#include <cstdint>
#include <limits>

namespace graehl {

#if __cplusplus >= 201103L
typedef std::uint64_t uint64_t;
typedef std::uint32_t uint32_t;
typedef std::uint16_t uint16_t;
typedef std::uint8_t uint8_t;

typedef std::int64_t int64_t;
typedef std::int32_t int32_t;
typedef std::int16_t int16_t;
typedef std::int8_t int8_t;
#else
using boost::int8_t;
using boost::uint8_t;
using boost::int16_t;
using boost::uint16_t;
using boost::int32_t;
using boost::uint32_t;
using boost::int64_t;
using boost::uint64_t;
#endif

template <class T>
struct signed_for_int {
  typedef T original_t;
  typedef T signed_t;
  typedef unsigned unsigned_t;
  enum { toa_bufsize = 20 };
};


#define GRAEHL_DEFINE_SIGNED_FOR_3(t, it, ut)                                                           \
  template <>                                                                                           \
  struct signed_for_int<t> {                                                                            \
    typedef ut unsigned_t;                                                                              \
    typedef it signed_t;                                                                                \
    typedef t original_t;                                                                               \
    enum { toa_bufsize = 3 + std::numeric_limits<t>::digits10, toa_bufsize_minus_1 = toa_bufsize - 1 }; \
  };

// toa_bufsize will hold enough chars for a c string converting to sign, digits (for both signed and unsigned
// types), because normally an unsigned would only need 2 extra chars. we reserve 3 explicitly for the case
// that itoa(buf, UINT_MAX, true) is called, with output +4......

#define GRAEHL_DEFINE_SIGNED_FOR(it) \
  GRAEHL_DEFINE_SIGNED_FOR_3(it, it, u##it) GRAEHL_DEFINE_SIGNED_FOR_3(u##it, it, u##it)
#define GRAEHL_DEFINE_SIGNED_FOR_NS(ns, it)             \
  GRAEHL_DEFINE_SIGNED_FOR_3(ns::it, ns::it, ns::u##it) \
  GRAEHL_DEFINE_SIGNED_FOR_3(ns::u##it, ns::it, ns::u##it)
#define GRAEHL_DEFINE_SIGNED_FOR_2(sig, unsig) \
  GRAEHL_DEFINE_SIGNED_FOR_3(sig, sig, unsig) GRAEHL_DEFINE_SIGNED_FOR_3(unsig, sig, unsig)

GRAEHL_DEFINE_SIGNED_FOR(int8_t)
GRAEHL_DEFINE_SIGNED_FOR(int16_t)
GRAEHL_DEFINE_SIGNED_FOR(int32_t)
GRAEHL_DEFINE_SIGNED_FOR(int64_t)

#if GRAEHL_INT_DIFFERENT_FROM_INTN
GRAEHL_DEFINE_SIGNED_FOR_2(int, unsigned)
#if GRAEHL_HAVE_LONGER_LONG
GRAEHL_DEFINE_SIGNED_FOR_2(long int, long unsigned)
#endif
#endif
#if GRAEHL_PTRDIFF_DIFFERENT_FROM_INTN
GRAEHL_DEFINE_SIGNED_FOR_2(std::ptrdiff_t, std::size_t)
#endif

#if GRAEHL_INT_DIFFERENT_FROM_INTN
#define GRAEHL_FOR_INT_DIFFERENT_FROM_INTN_TYPES(x) x(int) x(unsigned)
#else
#define GRAEHL_FOR_INT_DIFFERENT_FROM_INTN_TYPES(x)
#endif


#if GRAEHL_PTRDIFF_DIFFERENT_FROM_INTN
#define GRAEHL_FOR_PTRDIFF_DIFFERENT_FROM_INTN_TYPES(x) x(std::ptrdiff_t) x(std::size_t)
#else
#define GRAEHL_FOR_PTRDIFF_DIFFERENT_FROM_INTN_TYPES(x)
#endif

#if GRAEHL_HAVE_LONGER_LONG
#define GRAEHL_FOR_LONGER_LONG_TYPES(x) x(long) x(long unsigned)
#else
#define GRAEHL_FOR_LONGER_LONG_TYPES(x)
#endif

#define GRAEHL_FOR_DISTINCT_INT_TYPES(x)                                                    \
  x(uint8_t) x(uint16_t) x(uint32_t) x(uint64_t) x(int8_t) x(int16_t) x(int32_t) x(int64_t) \
      GRAEHL_FOR_INT_DIFFERENT_FROM_INTN_TYPES(x) GRAEHL_FOR_LONGER_LONG_TYPES(x)           \
          GRAEHL_FOR_PTRDIFF_DIFFERENT_FROM_INTN_TYPES(x)


#if GRAEHL_HAVE_LONG_DOUBLE
#define GRAEHL_FOR_DISTINCT_FLOAT_TYPES_LONG_DOUBLE(x) x(long double)
#else
#define GRAEHL_FOR_DISTINCT_FLOAT_TYPES_LONG_DOUBLE(x)
#endif

#define GRAEHL_FOR_DISTINCT_FLOAT_TYPES(x) x(float) x(double) GRAEHL_FOR_DISTINCT_FLOAT_TYPES_LONG_DOUBLE(x)


}

#endif
