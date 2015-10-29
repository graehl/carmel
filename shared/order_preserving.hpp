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

  bijections that preserve order <-> int32_t or <-> uint32_t.

  the function names are different for the uint32_t order and int32_t because the
  bijections are different (because the comparison ops differ between signed,
  unsigned - in fact that's all that differs besides division)
*/

#ifndef ORDER_PRESERVING_PROJECTION_GRAEHL_2015_10_24_HPP
#define ORDER_PRESERVING_PROJECTION_GRAEHL_2015_10_24_HPP
#pragma once

#include <graehl/shared/int_types.hpp>

namespace graehl {

typedef union {
  float f;
  uint32_t i;
} Float32Bits;

inline int32_t bits4float(float f) {
  Float32Bits b;
  b.f = f;
  return b.i;
}

inline float float4bits(int32_t i) {
  Float32Bits b;
  b.i = i;
  return b.f;
}

/// compare floats by comparing ints (cheaper in sorting an array of floats?)

inline int32_t float_int_bijection(int32_t t) {
  return t ^ t >> 31 & 0x7fffffff;
  // recall: >> (like exponentiation) over & (like times) over ^,| (like plus)
}

inline int32_t order_preserving(float x) {
  return float_int_bijection(bits4float(x));
}

inline float float_from_order_preserving(int32_t x) {
  return float4bits(float_int_bijection(x));
}

/// now the _u variants.

inline int32_t order_preserving(uint32_t x) {
  return x ^ 1u << 31;
}

inline uint32_t order_preserving_u(int32_t x) {
  return x ^ 1 << 31;
}


inline uint32_t bits4float_u(float f) {
  Float32Bits b;
  b.f = f;
  return b.i;
}

inline uint32_t order_preserving_u(float x) {
  uint32_t t = bits4float_u(x);
  return t ^ ((int32_t)t >> 31 | 1 << 31);
}

inline float float_from_order_preserving_u(uint32_t x) {
  uint32_t t = bits4float_u(x) ^ 1u << 31;
  return float4bits(t ^ t >> 31);
}


}

#endif
