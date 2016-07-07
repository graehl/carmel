// Copyright (c) 2014 Google, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// FarmHash, by Geoff Pike

//
// http://code.google.com/p/farmhash/
/** \file

  simplified (+ _MSC_VER works) implementation of Google Farmhash 'farmhashna'
  (arch-dependent hashing for hash tables). TODO: detect microarchitecture and
  use. full FarmHash has more special cases depending on sse4.2 etc that only
  matter for hashing very long strings (512+ bytes).


*/

#ifndef FARMHASH_GRAEHL_2015_10_25_HPP
#define FARMHASH_GRAEHL_2015_10_25_HPP
#pragma once

#ifndef GRAEHL_FETCH_UNALIGNED_MEMCPY
#define GRAEHL_FETCH_UNALIGNED_MEMCPY 0
#endif

#ifndef GRAEHL_FARMHASH_LIKELY_SMALL
#define GRAEHL_FARMHASH_LIKELY_SMALL 1
#endif

/// full-inline probably kills icache (though it's at end of fn so maybe not)
#define GRAEHL_FARMHASH_INLINE 0

#if GRAEHL_FARMHASH_LIKELY_SMALL
#include <graehl/shared/likely.hpp>
#define GRAEHL_FARMHASH_LIKELY_SMALL_EXPECT(x) likely_true(x)
#else
#define GRAEHL_FARMHASH_LIKELY_SMALL_EXPECT(x) (x)
#endif

#ifndef GRAEHL_FARMHASH_DEFINE_IMPL
#if defined(GRAEHL__SINGLE_MAIN) || (defined(GRAEHL__FARMHASH_MAIN) && GRAEHL__FARMHASH_MAIN)
#define GRAEHL_FARMHASH_DEFINE_IMPL 1
#else
#define GRAEHL_FARMHASH_DEFINE_IMPL 0
#endif
#endif

#include <graehl/shared/cpp11.hpp>
#include <graehl/shared/int_types.hpp>
#include <string>
#if GRAEHL_CPP11
#include <utility>
#else
#include <algorithm>
#endif


namespace graehl {

// Some primes between 2^63 and 2^64 for various uses.

GRAEHL_STATIC_CONSTEXPR uint64_t k_bigprime0 = GRAEHL_BIG_CONSTANT(0xc3a5c85c97cb3127);
GRAEHL_STATIC_CONSTEXPR uint64_t k_bigprime1 = GRAEHL_BIG_CONSTANT(0xb492b66fbe98f273);
GRAEHL_STATIC_CONSTEXPR uint64_t k_bigprime2 = GRAEHL_BIG_CONSTANT(0x9ae16a3b2f90404f);
GRAEHL_STATIC_CONSTEXPR uint64_t k_bigprime3 = GRAEHL_BIG_CONSTANT(0x9ddfea08eb382d69);

GRAEHL_FORCE_INLINE uint64_t fast_mixbits(uint64_t v) {
  return v ^ (v >> 47);
}

/// \hash exactly 16 bytes uv
GRAEHL_FORCE_INLINE uint64_t farmhash_len_16(uint64_t u, uint64_t v) {
  // Murmur-inspired hashing.
  uint64_t a = (u ^ v) * k_bigprime3;
  a ^= (a >> 47);
  uint64_t b = (v ^ a) * k_bigprime3;
  b ^= (b >> 47);
  b *= k_bigprime3;
  return b;
}

/// \hash exactly 16 bytes uv w/ prime (or at least odd) mul
GRAEHL_FORCE_INLINE uint64_t farmhash_len_16(uint64_t u, uint64_t v, uint64_t mul) {
  uint64_t a, b;
  a = (u ^ v) * mul;
  a ^= (a >> 47);
  b = (v ^ a) * mul;
  b ^= (b >> 47);
  b *= mul;
  return b;
}

/// short strings are hashed here.
GRAEHL_FORCE_INLINE uint64_t farmhash_len_0to16(char const* s, std::size_t len) {
  if (len >= 8) {
    uint64_t mul = k_bigprime2 + len * 2;  // odd
    uint64_t a = fetch_uint64(s) + k_bigprime2;
    uint64_t b = fetch_uint64(s + len - 8);
    uint64_t c = bit_rotate_right_64(b, 37) * mul + a;
    uint64_t d = (bit_rotate_right_64(a, 25) + b) * mul;
    return farmhash_len_16(c, d, mul);
  } else if (len >= 4) {
    uint64_t mul = k_bigprime2 + len * 2;  // odd
    uint64_t a = fetch_uint32(s);
    return farmhash_len_16(len + (a << 3), fetch_uint32(s + len - 4), mul);
  } else if (len > 0) {
    uint8_t a = s[0];
    uint8_t b = s[len >> 1];
    uint8_t c = s[len - 1];
    uint32_t y = (uint32_t)a + ((uint32_t)b << 8);
    uint32_t z = len + ((uint32_t)c << 2);
    return fast_mixbits(y * k_bigprime2 ^ z * k_bigprime0) * k_bigprime2;
  } else
    return k_bigprime2;
}

GRAEHL_FORCE_INLINE uint64_t farmhash_len_16to32(char const* s, std::size_t len) {
  uint64_t mul = k_bigprime2 + len * 2;  // odd
  uint64_t a = fetch_uint64(s) * k_bigprime1;
  uint64_t b = fetch_uint64(s + 8);
  uint64_t c = fetch_uint64(s + len - 8) * mul;
  uint64_t d = fetch_uint64(s + len - 16) * k_bigprime2;
  return farmhash_len_16(bit_rotate_right_64(a + b, 43) + bit_rotate_right_64(c, 30) + d,
                         a + bit_rotate_right_64(b + k_bigprime2, 18) + c, mul);
}

GRAEHL_FORCE_INLINE uint64_t farmhash_len_32to64(char const* s, std::size_t len) {
  uint64_t mul = k_bigprime2 + len * 2;  // odd
  uint64_t a = fetch_uint64(s) * k_bigprime2;
  uint64_t b = fetch_uint64(s + 8);
  uint64_t c = fetch_uint64(s + len - 8) * mul;
  uint64_t d = fetch_uint64(s + len - 16) * k_bigprime2;
  uint64_t y = bit_rotate_right_64(a + b, 43) + bit_rotate_right_64(c, 30) + d;
  uint64_t z = farmhash_len_16(y, a + bit_rotate_right_64(b + k_bigprime2, 18) + c, mul);
  uint64_t e = fetch_uint64(s + 16) * mul;
  uint64_t f = fetch_uint64(s + 24);
  uint64_t g = (y + fetch_uint64(s + len - 32)) * mul;
  uint64_t h = (z + fetch_uint64(s + len - 24)) * mul;
  return farmhash_len_16(bit_rotate_right_64(e + f, 43) + bit_rotate_right_64(g, 30) + h,
                         e + bit_rotate_right_64(f + a, 18) + g, mul);
}

#if !GRAEHL_FARMHASH_INLINE
uint64_t farmhash_long(char const* s, std::size_t len);
#endif

#if GRAEHL_FARMHASH_DEFINE_IMPL || GRAEHL_FARMHASH_INLINE

// POD (unlike std::pair<uint64_t>)
struct Uint128p {
  uint64_t first;
  uint64_t second;
};

/// \return a 16-byte hash for 32bit wxyz, plus 16 bytes of seeds ab. Quick and dirty.
GRAEHL_FORCE_INLINE Uint128p weak_farmhash_len_32_with_seeds(uint64_t w, uint64_t x, uint64_t y, uint64_t z,
                                                             uint64_t a, uint64_t b) {
  Uint128p result;
  uint64_t c;
  a += w;
  b = bit_rotate_right_64(b + a + z, 21);
  c = a;
  a += x;
  a += y;
  b += bit_rotate_right_64(a, 44);
  result.first = a + z;
  result.second = b + c;
  return result;
}

/// \return a 16-byte hash for s[0] ... s[31], 16 bytes of seeds ab.  Quick and dirty (a and b should be
/// 'random'-ish)
GRAEHL_FORCE_INLINE Uint128p weak_farmhash_len_32_with_seeds(char const* s, uint64_t a, uint64_t b) {
  return weak_farmhash_len_32_with_seeds(fetch_uint64(s), fetch_uint64(s + 8), fetch_uint64(s + 16),
                                         fetch_uint64(s + 24), a, b);
}

#if GRAEHL_FARMHASH_INLINE
static inline
#endif
    uint64_t
    farmhash_long(char const* s, std::size_t len) {
  const uint64_t seed = 81;
  uint64_t x = seed, y = seed * k_bigprime1 + 113, z = fast_mixbits(y * k_bigprime2 + 113) * k_bigprime2;
#if GRAEHL_CPP11
  Uint128p v = {0, 0}, w = {0, 0};
#else
  Uint128p v, w;
  v.first = v.second = w.first = w.second = 0;
#endif
  x = x * k_bigprime2 + fetch_uint64(s);

  char const* end = s + ((len - 1) / 64) * 64;
  char const* last64 = end + ((len - 1) & 63) - 63;

  do {
    x = bit_rotate_right_64(x + y + v.first + fetch_uint64(s + 8), 37) * k_bigprime1;
    y = bit_rotate_right_64(y + v.second + fetch_uint64(s + 48), 42) * k_bigprime1;
    x ^= w.second;
    y += v.first + fetch_uint64(s + 40);
    z = bit_rotate_right_64(z + w.first, 33) * k_bigprime1;
    v = weak_farmhash_len_32_with_seeds(s, v.second * k_bigprime1, x + w.first);
    w = weak_farmhash_len_32_with_seeds(s + 32, z + w.second, y + fetch_uint64(s + 16));
    std::swap(z, x);
    s += 64;
  } while (s != end);

  uint64_t mul = k_bigprime1 + ((z & 0xff) << 1);
  s = last64;
  w.first += ((len - 1) & 63);
  v.first += w.first;
  w.first += v.first;
  x = bit_rotate_right_64(x + y + v.first + fetch_uint64(s + 8), 37) * mul;
  y = bit_rotate_right_64(y + v.second + fetch_uint64(s + 48), 42) * mul;
  x ^= w.second * 9;
  y += v.first * 9 + fetch_uint64(s + 40);
  z = bit_rotate_right_64(z + w.first, 33) * mul;
  v = weak_farmhash_len_32_with_seeds(s, v.second * mul, x + w.first);
  w = weak_farmhash_len_32_with_seeds(s + 32, z + w.second, y + fetch_uint64(s + 16));

  return farmhash_len_16(farmhash_len_16(v.first, w.first, mul) + fast_mixbits(y) * k_bigprime0 + x,
                         farmhash_len_16(v.second, w.second, mul) + z, mul);
}
#endif

/// this shouldn't kill icache for common case (short string), even though we've
/// inlined insanely - the short case exits immediately
GRAEHL_FORCE_INLINE
uint64_t farmhash(char const* s, std::size_t len) {
  if (GRAEHL_FARMHASH_LIKELY_SMALL_EXPECT(len <= 32)) {
    return len <= 16 ? farmhash_len_0to16(s, len) : farmhash_len_16to32(s, len);
  } else if (len <= 64)
    return farmhash_len_32to64(s, len);
  else
    return farmhash_long(s, len);
}

template <class StringView>
GRAEHL_FORCE_INLINE uint64_t farmhash_str(StringView const& s) {
  return farmhash(s.data(), s.length());
}

struct Farmhash {
  typedef std::size_t result_type;

  /// hash fn object
  template <class StringView>
  std::size_t operator()(StringView const& s) const {
    return farmhash(s.data(), s.length());
  }

  /// for tbb::concurrent_hash_map
  template <class StringView>
  static inline std::size_t hash(StringView const& s) {
    return farmhash(s.data(), s.length());
  }
  template <class StringView1, class StringView2>
  static inline bool equal(StringView1 const& a, StringView2 const& b) {
    return a == b;
  }
};


}

#endif
