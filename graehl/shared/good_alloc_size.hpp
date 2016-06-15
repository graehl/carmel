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

    avoid wasting memory or wasting cpu cache by awareness of allocator and cache granularity
*/

#ifndef GOOD_ALLOC_SIZE_JG_2014_08_30_HPP
#define GOOD_ALLOC_SIZE_JG_2014_08_30_HPP
#pragma once

namespace graehl {

template <unsigned divisor>
struct divide_round_up {
  enum { div = divisor };
  template <class Size>
  static inline Size divided(Size sz) {
    return (sz + (divisor - 1)) / divisor;
  }
};

template <std::size_t request, std::size_t multiple_pow2>
struct rounded_up_multiple_pow2 {
  enum { multiple_of = multiple_pow2 };
  enum { mask = multiple_of - 1 };
  enum { value = (request + mask) & ~mask };
};

template <class Size>
inline bool is_pow2(Size x) {
  return (x & (x - 1)) == 0;
}

/// return multiple of divisor >= req. divisor must be a power of 2
template <class Size>
inline Size round_up_pow2(Size req, Size divisor_pow2) {
  assert(is_pow2(divisor_pow2));
  --divisor_pow2;
  return (req + divisor_pow2) & ~divisor_pow2;
}

/// must be power of 2
enum { kcache_line = 64 };
enum { kcache_line_mask = kcache_line - 1 };

/// must be power of 2
enum { ktarget_first_alloc = kcache_line };
enum { ktarget_first_alloc_mask = ktarget_first_alloc - 1 };
enum { k256 = 256 };
enum { kmax_round_to_cache_line = 512 };
enum { k1K = 1024 };
enum { k4K = 4 * k1K };
enum { k1M = k1K * k1K };
enum { k4M = 4 * k1M };

template <class Size>
inline Size good_alloc_size(Size req) {
  if (req <= ktarget_first_alloc)
    return ktarget_first_alloc;
  else if (req <= kmax_round_to_cache_line)
    return (req + kcache_line_mask) & ~kcache_line_mask;
  else if (req <= (k4K - k256))  //
    return round_up_pow2(req, (Size)k256);
  else if (req <= 4072 * k1K)  // nearly 4mb
    return round_up_pow2(req, (Size)k4K);
  else
    // TODO: test
    return round_up_pow2(req, (Size)k4M);
}

/// double for very large allocations, else 3/2
template <class Size>
inline Size next_alloc_target(Size req) {
  // TODO: test
  if (req <= 128 * (Size)k1K)
    return (req * 3 + 1) / 2;
  else {
    Size const twice = req * 2;
    return twice > req ? twice : (Size)-1;
  }
}

template <class Size>
inline Size next_good_alloc_size(Size req) {
  // TODO: test
  return good_alloc_size(next_alloc_target(req));
}

template <class T>
struct good_vector_size {
  enum { ktarget_first_sz = ktarget_first_alloc / sizeof(T) };
  /// returns at least prev
  template <class Size>
  static Size want(Size req) {
    if (req * sizeof(T) <= 128 * (Size)k1K)
      return (req * 3 + 1) / 2;
    else {
      // TODO: test
      if (sizeof(Size) > 4)
        return req * 2;
      else {
        // (prevents overflowing 2G -> 0 for 4 byte Size
        Size const twice = req * 2;
        return twice > req ? twice : (Size)-1;
      }
    }
  }
  template <class Size>
  static Size next(Size req) {
    return req <= ktarget_first_sz ? ktarget_first_sz : good_alloc_size(sizeof(T) * want(req)) / sizeof(T);
  }
};


}

#endif
