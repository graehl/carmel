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

   .
*/

#ifndef GRAEHL_SHARED__INT_HASH_MAP_HPP
#define GRAEHL_SHARED__INT_HASH_MAP_HPP
#pragma once

#include <graehl/shared/containers.hpp>
#include <graehl/shared/int_types.hpp>
#include <vector>

namespace graehl {


// not a template member because we don't want pointer casting (e.g. void *) to change hash val
template <class P>
struct ptr_hash {
  std::size_t operator()(void* p) const { return (P const*)p - (P const*)0; }
};

GRAEHL_CONSTEXPR uint32_t golden_ratio_fraction_32u = 2654435769U;  // (floor of 2^32/golden_ratio)

inline void uint32_hash_inplace(uint32_t& a) {
  a *= golden_ratio_fraction_32u;  // mixes the lower bits into the upper bits
  a ^= (a >> 16);  // gets some of the goodness back into the lower bits
}

inline uint32_t uint32_hash_fast(uint32_t a) {
  uint32_hash_inplace(a);
  return a;
}

inline void mix_hash_inplace(uint64_t& a, uint64_t b) {
  a ^= bit_rotate_left(b, 1);
}

inline uint64_t mix_hash_fast(uint64_t a, uint64_t b) {
  a ^= bit_rotate_left(b, 1);
  return a;
}

GRAEHL_CONSTEXPR uint64_t prime_64bit = GRAEHL_BIG_CONSTANT(0x2127599bf4325c37);
/// 32 bit result. use this for collections of smaller than 4 billion items.
struct int_hash_32 {
  unsigned operator()(std::size_t x) const {
    // (a large odd number - invertible)
    return (x * prime_64bit) >> 32;
    // this way we get at least some of the msb into the lsbs (and of course
    // lsbs toward the msbs. i don't care that the top 24 bits are 0. if you
    // care, you can add x to the result. no hash table will be that large

    // the largest 64-bit prime: 18446744073709551557ULL
  }
};

struct int_hash {
  std::size_t operator()(std::size_t h) const {
    h ^= h >> 23;
    h *= prime_64bit;
    h ^= h >> 47;
    return h;
  }
};

template <class Val>
struct int_hash_map : GRAEHL_UNORDERED_NS::unordered_map<std::size_t, Val, int_hash_32> {
  typedef GRAEHL_UNORDERED_NS::unordered_map<std::size_t, Val, int_hash> Base;
  int_hash_map(std::size_t reserve = 1000, float load_factor = .75) : Base(reserve) {
    this->max_load_factor(load_factor);
  }
};

typedef int_hash_map<void*> int_ptr_map;

template <class Val>
struct direct_int_map {
  std::vector<Val> vals;
  direct_int_map(std::size_t reserve = 1000) : vals(reserve) {}
  Val& operator[](std::size_t i) {
    if (i < vals.size()) return vals[i];
    vals.resize(i + 1);
    return vals[i];
  }
};

typedef direct_int_map<void*> direct_int_ptr_map;


}

#endif
