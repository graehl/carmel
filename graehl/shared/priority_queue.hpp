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

#ifndef GRAEHL_SHARED__PRIORITY_QUEUE_HPP
#define GRAEHL_SHARED__PRIORITY_QUEUE_HPP
#pragma once

#include <graehl/shared/d_ary_heap.hpp>
#include <functional>

namespace graehl {

/// see d_ary_heap_indirect for public methods push(val), pop(), Val const& top()), adjust_top()
/// Equal is used by queue.contains(value) which only works if IndexInHeapPropertyMap is real
template <class ValueContainer,
          std::size_t Arity = 4  // 4 is fastest for small objects (cache line usage). experiment with 2...8
          ,
          class DistanceMap = identity_distance<typename ValueContainer::value_type>,
          class Better = std::less<typename DistanceMap::value_type>,
          class IndexInHeapPropertyMap
          = no_index_in_heap<typename ValueContainer::value_type, typename ValueContainer::size_type>,
          class Size = typename ValueContainer::size_type,
          class Equal = std::equal_to<typename ValueContainer::value_type> >
struct priority_queue : d_ary_heap_indirect<typename ValueContainer::value_type, Arity, DistanceMap,
                                            IndexInHeapPropertyMap, Better, ValueContainer, Size, Equal> {
  typedef d_ary_heap_indirect<typename ValueContainer::value_type, Arity, DistanceMap, IndexInHeapPropertyMap,
                              Better, ValueContainer, Size, Equal> Base;
  priority_queue(DistanceMap const& distance,
                 IndexInHeapPropertyMap const& indexInHeap = IndexInHeapPropertyMap(),
                 Better const& better = Better(), Size containerReserve = 100, Equal const& equal = Equal())
      : Base(distance, indexInHeap, better, containerReserve, equal) {}
  /// see d_ary_heap_indirect for public methods (push(val), pop(), Val const& top()), adjust_top()
  priority_queue(Size containerReserve = 1 + Arity + Arity * Arity)
      : Base(DistanceMap(), IndexInHeapPropertyMap(), Better(), containerReserve, Equal()) {}
};


}

#endif
