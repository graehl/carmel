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

 for d_ary_heap experiments.
*/

#ifndef ALIGNED_ALLOCATOR_JG_2015_04_20_HPP
#define ALIGNED_ALLOCATOR_JG_2015_04_20_HPP
#pragma once

#include <memory>

namespace graehl {

// TODO: pre-alloc header for exact alignment
template <class T, unsigned OffsetPlus1 = 1>
struct aligned_allocator : public std::allocator<T> {
  enum { offset = OffsetPlus1-1 };
  typedef std::size_t size_type;
  typedef std::size_t offset_type;
  typedef T* pointer;
  typedef const T* const_pointer;
  template <class T2>
  struct rebind {
    typedef aligned_allocator<T2> other;
  };
  pointer allocate(size_type n, const void* hint = 0) {
    return std::allocator<T>::allocate(n + offset, hint) + offset;
  }
  void deallocate(pointer p, size_type n) { std::allocator<T>::deallocate(p-offset, n); }
};


}

#endif
