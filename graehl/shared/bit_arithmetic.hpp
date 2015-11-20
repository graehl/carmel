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

    tricky+efficient integer bit manipulations.
*/


#ifndef GRAEHL__SHARED__BIT_ARITHMETIC_HPP
#define GRAEHL__SHARED__BIT_ARITHMETIC_HPP
#pragma once

#include <graehl/shared/type_traits.hpp>
#include <graehl/shared/int_types.hpp>
#include <cassert>
#include <limits>
#include <limits.h>

namespace graehl {

inline void mixbits(uint64_t& h) {
  h ^= h >> 23;
  h *= GRAEHL_BIG_CONSTANT(0x2127599bf4325c37);
  h ^= h >> 47;
}

// bit i=0 = lsb
template <class I, class J>
inline void set(typename enable_if<is_integral<I>::value>::type& bits, J i) {
  assert(i < (CHAR_BIT * sizeof(I)));
  I mask = (1 << i);
  bits |= mask;
}

template <class I>
inline void set_mask(I& bits, I mask) {
  bits |= mask;
}

template <class I, class J>
inline void reset(typename enable_if<is_integral<I>::value>::type& bits, J i) {
  assert(i < (CHAR_BIT * sizeof(I)));
  I mask = (1 << i);
  bits &= ~mask;
}

template <class I>
inline void reset_mask(I& bits, I mask) {
  bits &= ~mask;
}

template <class I, class J>
inline void set(typename enable_if<is_integral<I>::value>::type& bits, J i, bool to) {
  assert(i < (CHAR_BIT * sizeof(I)));
  I mask = (1 << i);
  if (to)
    set_mask(bits, mask);
  else
    reset_mask(bits, mask);
}

template <class I>
inline void set_mask(I& bits, I mask, bool to) {
  if (to)
    set_mask(bits, mask);
  else
    reset_mask(bits, mask);
}

template <class I, class J>
inline bool test(typename enable_if<is_integral<I>::value>::type bits, J i) {
  assert(i < (CHAR_BIT * sizeof(I)));
  I mask = (1 << i);
  return mask & bits;
}

// if any of mask
template <class I>
inline bool test_mask(typename enable_if<is_integral<I>::value>::type bits, I mask) {
  return mask & bits;
}

// return true if was already set, then set.
template <class I, class J>
inline bool latch(typename enable_if<is_integral<I>::value>::type& bits, J i) {
  assert(i < (CHAR_BIT * sizeof(I)));
  I mask = (1 << i);
  bool r = mask & bits;
  bits |= mask;
  return r;
}

template <class I>
inline bool latch_mask(typename enable_if<is_integral<I>::value>::type& bits, I mask) {
  bool r = mask & bits;
  bits |= mask;
  return r;
}

template <class I>
inline bool test_mask_all(typename enable_if<is_integral<I>::value>::type bits, I mask) {
  return (mask & bits) == mask;
}

//
// the reason for the remove_cv stuff is that the compiler wants to turn
// the call
//    size_t x;
//    bit_rotate(x,3)
// into an instantiation of
//    template<class size_t&, int const>
//    size_t& bit_rotate_left(size_t&,int const);
//

template <class I, class J>
inline typename enable_if<is_integral<I>::value, typename remove_cv<I>::type>::type bit_rotate_left(I x, J k) {
  typedef typename remove_cv<I>::type IT;
  assert(k < std::numeric_limits<IT>::digits);
  assert(std::numeric_limits<IT>::digits == CHAR_BIT * sizeof(IT));
  return ((x << k) | (x >> (std::numeric_limits<IT>::digits-k)));
}

template <class I, class J>
inline typename enable_if<is_integral<I>::value, typename remove_cv<I>::type>::type bit_rotate_right(I x, J k) {
  typedef typename remove_cv<I>::type IT;
  assert(k < std::numeric_limits<IT>::digits);
  assert(std::numeric_limits<IT>::digits == CHAR_BIT * sizeof(IT));
  return ((x << (std::numeric_limits<IT>::digits-k)) | (x >> k));
}

/// interpret the two bytes at d as a uint16 in little endian order
inline uint16_t unpack_uint16_little(void const* d) {
// FIXME: test if the #ifdef optimization is even needed (compiler may optimize portable to same?)
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) || defined(_MSC_VER) \
    || defined(__BORLANDC__) || defined(__TURBOC__)
  return *((const uint16_t*)(d));
#else
  return ((((uint32_t)(((const uint8_t*)(d))[1])) << CHAR_BIT) + (uint32_t)(((const uint8_t*)(d))[0]));
#endif
}


}

#endif
