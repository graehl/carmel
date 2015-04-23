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

   .
*/

#ifndef GRAEHL_SHARED__INT_HASH_MAP_HPP
#define GRAEHL_SHARED__INT_HASH_MAP_HPP
#pragma once

//reminder: undefined macros eval to 0
#if __cplusplus >= 201103L  || defined(__clang__) && defined(_LIBCPP_VERSION)
# include <unordered_map>
# define GRAEHL_UNORDERED_NS std
#elif _MSC_VER >= 1500 && _HAS_TR1 || _MSC_VER >= 1700
# include <unordered_map>
# define GRAEHL_UNORDERED_NS std::tr1
#elif __GNUC__ == 4 && __GNUC__MINOR__ >= 2
# include <tr1/unordered_map>
# define GRAEHL_UNORDERED_NS std::tr1
#else
# include <boost/unordered_map.hpp>
# define GRAEHL_UNORDERED_NS boost
#endif

#include <vector>
#include <cstddef>

namespace graehl {

/// 32 bit result. use this for collections of smaller than 4 billion items.
struct int_hash_32 {
  inline unsigned operator()(std::size_t x) const {
    // (a large odd number - invertible)
    return (x * 0x2127599bf4325c37ULL) >> 32;
    // this way we get at least some of the msb into the lsbs (and of course
    // lsbs toward the msbs. i don't care that the top 24 bits are 0. if you
    // care, you can add x to the result. no hash table will be that large

    // the largest 64-bit prime: 18446744073709551557ULL
  }
};

struct int_hash {
  inline std::size_t operator()(std::size_t h) const {
    h ^= h >> 23;
    h *= 0x2127599bf4325c37ULL;
    h ^= h >> 47;
    return h;
  }
};

template <class Val>
struct int_hash_map : GRAEHL_UNORDERED_NS::unordered_map<std::size_t, Val, int_hash_32> {
  typedef GRAEHL_UNORDERED_NS::unordered_map<std::size_t, Val, int_hash> Base;
  int_hash_map(std::size_t reserve = 1000, float load_factor = .75)
      : Base(reserve)
  {
    this->max_load_factor(load_factor);
  }
};

typedef int_hash_map<void *> int_ptr_map;

template <class Val>
struct direct_int_map {
  std::vector<Val> vals;
  direct_int_map(std::size_t reserve = 1000)
      : vals(reserve)
  {}
  Val &operator[](std::size_t i) {
    if (i < vals.size())
      return vals[i];
    vals.resize(i+1);
    return vals[i];
  }
};

typedef direct_int_map<void *> direct_int_ptr_map;


}

#endif
