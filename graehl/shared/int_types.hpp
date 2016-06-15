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

    for templates: jump between unsigned and signed int types of same width
*/

#ifndef INT_TYPES_JG2012531_HPP
#define INT_TYPES_JG2012531_HPP
#pragma once

#include <graehl/shared/cpp11.hpp>

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


/// on recent (x86) architectures aligned reads are fine
#define GRAEHL_FETCH_UNALIGNED_MEMCPY 0

#ifndef HAVE_BUILTIN_CLZ
#define HAVE_BUILTIN_CLZ 1
#endif

#ifndef HAVE_BUILTIN_POPCNT
#define HAVE_BUILTIN_POPCNT 1
#endif

#ifdef _MSC_VER
// TODO:
#undef HAVE_BUILTIN_CLZ
#define HAVE_BUILTIN_CLZ 0

// TODO: test 0
#undef GRAEHL_FETCH_UNALIGNED_MEMCPY
#define GRAEHL_FETCH_UNALIGNED_MEMCPY 1
#endif


#if GRAEHL_FETCH_UNALIGNED_MEMCPY
#define GRAEHL_FETCH32(p) graehl::fetch_uint32(p)
#define GRAEHL_FETCH64(p) graehl::fetch_uint64(p)
#else
#define GRAEHL_FETCH32(p) (*reinterpret_cast<graehl::uint32_t const*>(p))
#define GRAEHL_FETCH64(p) (*reinterpret_cast<graehl::uint64_t const*>(p))
#endif


#if HAVE_BUILTIN_POPCNT
#ifdef _MSC_VER
#include <intrin.h>
#define __builtin_popcount __popcnt
#define __builtin_popcountl __popcnt64
#define __builtin_popcountll __popcnt64
#endif
#endif


#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

#if defined(_MSC_VER)

#define GRAEHL_FORCE_INLINE static __forceinline
#include <stdlib.h>
#define GRAEHL_ROTL32(x, shift) _rotl(x, shift)
#define GRAEHL_ROTL64(x, shift) _rotl64(x, shift)
#define GRAEHL_ROTR32(x, shift) _rotr(x, shift)
#define GRAEHL_ROTR64(x, shift) _rotr64(x, shift)
#define GRAEHL_BIG_CONSTANT(x) (x)

#else

#define GRAEHL_FORCE_INLINE static inline __attribute__((always_inline))

/// as always for macros expanding an argument twice, use fn calls
/// bit_rotate_left64 etc if arg x is not a simple value (e.g. side effects)
#define GRAEHL_ROTL32(x, shift) ((x) << shift) | ((x) >> (32 - shift))
#define GRAEHL_ROTL64(x, shift) ((x) << shift) | ((x) >> (64 - shift))
#define GRAEHL_ROTR32(x, shift) ((x) >> shift) | ((x) << (32 - shift))
#define GRAEHL_ROTR64(x, shift) ((x) >> shift) | ((x) << (64 - shift))
#define GRAEHL_BIG_CONSTANT(x) (x##LLU)
#endif


#if GRAEHL_CPP11
#if GRAEHL_CPP14
#define GRAEHL_STATIC_CONSTEXPR static constexpr const
#else
#define GRAEHL_STATIC_CONSTEXPR static constexpr
#endif
#include <cstdint>
#else
#define GRAEHL_STATIC_CONSTEXPR static const
#include <boost/cstdint.hpp>
#endif

#include <cassert>
#include <cstring>
#include <limits>

namespace graehl {

#if GRAEHL_CPP11
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
using boost::int16_t;
using boost::int32_t;
using boost::int64_t;

using boost::uint8_t;
using boost::uint16_t;
using boost::uint32_t;
using boost::uint64_t;
#endif


GRAEHL_FORCE_INLINE uint32_t fetch_uint32(char const* p) {
#if GRAEHL_FETCH_UNALIGNED_MEMCPY
  uint32_t x;
  std::memcpy(&x, p, sizeof(x));
  return x;
#else
  return *(reinterpret_cast<graehl::uint32_t const*>(p));
#endif
}

GRAEHL_FORCE_INLINE uint64_t fetch_uint64(char const* p) {
#if GRAEHL_FETCH_UNALIGNED_MEMCPY
  uint64_t x;
  std::memcpy(&x, p, sizeof(x));
  return x;
#else
  return *(reinterpret_cast<graehl::uint64_t const*>(p));
#endif
}

GRAEHL_FORCE_INLINE
uint32_t bit_rotate_left(uint32_t x, int8_t k) {
  return GRAEHL_ROTL32(x, k);
}

GRAEHL_FORCE_INLINE
uint64_t bit_rotate_left_64(uint64_t x, int8_t k) {
  return GRAEHL_ROTL64(x, k);
}

GRAEHL_FORCE_INLINE
uint32_t bit_rotate_right(uint32_t x, int8_t k) {
  return GRAEHL_ROTR32(x, k);
}

GRAEHL_FORCE_INLINE
uint64_t bit_rotate_right_64(uint64_t x, int8_t k) {
  return GRAEHL_ROTR64(x, k);
}

template <class Int>
inline bool is_power_of_2(Int i) {
  return (i & (i - 1)) == 0;
}

/// return power of 2 >= x
inline uint32_t next_power_of_2(uint32_t x) {
  if (!x) return 1;
#ifndef HAVE_BUILTIN_CLZ
  assert(sizeof(x) == sizeof(unsigned));
  return 1u << (32 - __builtin_clz(x - 1));
#else
  assert(x <= (1 << 30));
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  ++x;
  assert(is_power_of_2(x));
  return x;
#endif
}

/// return power of 2 >= x
inline uint64_t next_power_of_2(uint64_t x) {
  if (!x) return 1;
#if HAVE_BUILTIN_CLZ
  assert(sizeof(x) == sizeof(unsigned long));
  return 1u << (64 - __builtin_clzl(x - 1));
#else
  assert(x <= (1ULL << 60));
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x |= x >> 32;
  ++x;
  assert(is_power_of_2(x));
  return x;
#endif
}

#if GRAEHL_CPP11
/// slow but pure functional for constexpr. defaulted parameter is for impl only
inline constexpr uint64_t ceil_log2_const(uint64_t x, bool exact = true) {
  return (x == 0) ? (1 / x)
                  : (x == 1) ? (exact ? 0 : 1) : 1 + ceil_log2_const(x >> 1, ((x & 1) == 1) ? false : exact);
}

#if 0
inline constexpr uint64_t round_up_to_pow2_const(uint64_t x) {
  return (uint64_t)1 << ceil_log2_const(x);
}
#else
/// pure functional version of next_power_of_2
inline constexpr uint64_t next_power_of_2_const_r(uint64_t x, uint8_t shift) {
  return shift == 64 ? x : next_power_of_2_const_r(x | (x >> shift), shift * 2);
}

inline constexpr uint64_t next_power_of_2_const(uint64_t x) {
  return next_power_of_2_const_r(x-1, 1) + 1;
}
#endif
#endif

inline unsigned count_set_bits(uint32_t x) {
#if HAVE_BUILTIN_POPCNT
  return __builtin_popcount(x);
#else
  x = x - ((x >> 1) & 0x55555555);
  x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
  return (((x + (x >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
#endif
}

/// to avoid gcc 5 ambig from int -> uintX
inline unsigned count_set_bits(int32_t x) {
  return count_set_bits((uint32_t)x);
}

inline unsigned count_set_bits(uint64_t x) {
#if HAVE_BUILTIN_POPCNT
  return __builtin_popcountl(x);
#else
  return count_set_bits((uint32_t)x) + count_set_bits((uint32_t)(x >> 32));
#endif
}

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
