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

    C99 isnan isfinite isinf. //TODO: use actual C99 versions when available
*/

#ifndef GRAEHL_SHARED__NAN_HPP
#define GRAEHL_SHARED__NAN_HPP
#pragma once

#ifndef GRAEHL_HAVE_STD_ISINF
#define GRAEHL_HAVE_STD_ISINF 1
#endif

#include <cmath>

#if defined(_MSC_VER)
#define WIN32_NAN
#define GRAEHL_ISNAN(x) (_isnan(x) != 0)
#else
#if defined __FAST_MATH__
#define GRAEHL_ISNAN isnan
#if __cplusplus < 201103L
// gcc 4.x -ffast-math breaks std::isnan
#include <stdint.h>
#undef isnan
inline bool isnan(float f) {
  union {
    float f;
    uint32_t x;
  } u = {f};
  return (u.x << 1) > 0xff000000u;
}
inline bool isnan (double f)
{
  typedef unsigned long long u64;
  typedef long long i64;
  u64 j = *(i64*)&f & ~0x8000000000000000uLL;
  return j > 0x7ff0000000000000uLL;
}
// TODO: fix for long double also?
#endif
#else
#if defined(_STLPORT_VERSION) || defined(ANDROID)
#include <math.h>
#define GRAEHL_ISNAN(x) isnan(x)  // in stlport, only c99 version of isnan is available
#else
#include <cmath>
#define GRAEHL_ISNAN(x) std::isnan(x)  // gcc native stdlib includes isnan as an exported template function
#endif
#endif
#endif

#include <limits>

#ifdef WIN32_NAN
#include <float.h>
//# include <xmath.h>
namespace {
const unsigned graehl_nan[2] = {0xffffffff, 0x7fffffff};
}
#undef NAN
#define NAN (*(const double*)graehl_nan)
#endif

#ifndef NAN
#define NAN (0.0 / 0.0)
#endif

namespace graehl {

template <bool>
struct nan_static_assert;
template <>
struct nan_static_assert<true> {};

// is_iec559 i.e. only IEEE 754 float has x != x <=> x is nan
template <typename T>
inline bool is_nan(T x) {
  return GRAEHL_ISNAN(x);
}

template <typename T>
inline bool is_inf(T x) {
#if GRAEHL_HAVE_STD_ISINF
  return std::isinf(x);
#else
  return x == std::numeric_limits<T>::infinity() || x == -std::numeric_limits<T>::infinity();
#endif
}

template <typename T>
inline bool is_pos_inf(T x) {
  return x == std::numeric_limits<T>::infinity();
}

template <typename T>
inline bool is_neg_inf(T x) {
  return x == -std::numeric_limits<T>::infinity();
}

// c99 isfinite macro shoudl be much faster
template <typename T>
inline bool is_finite(T x) {
#if GRAEHL_HAVE_STD_ISINF
  return std::isfinite(x);
#else
  return !is_nan(x) && !is_inf(x);
#endif
}


}

#endif
