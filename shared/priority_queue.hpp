/** \file

    .
*/

#ifndef GRAEHL_SHARED__PRIORITY_QUEUE_HPP
#define GRAEHL_SHARED__PRIORITY_QUEUE_HPP

#include <xmt/graehl/shared/d_ary_heap.hpp>
#include <xmt/graehl/shared/property.hpp>
#include <functional>

namespace graehl {

/// see d_ary_heap_indirect for public methods push(val), pop(), Val const& top()), adjust_top()
template <class ValueContainer
          , std::size_t Arity = 4 // 4 is fastest for small objects (cache line usage). experiment with 2...8
          , class DistanceMap = IdentityMap<typename ValueContainer::value_type>
          , class IndexInHeapPropertyMap = NullPropertyMap<std::size_t> // don't track index
          , class Better = std::less<typename DistanceMap::value_type>
          , class Size = typename ValueContainer::size_type
          , class Equal = std::equal_to<typename ValueContainer::value_type>
          // used by queue.contains(value). if you don't call that, you can implement a fake ==
          >
struct priority_queue
    : d_ary_heap_indirect<typename ValueContainer::value_type
                          , Arity
                          , DistanceMap
                          , IndexInHeapPropertyMap
                          , Better
                          , ValueContainer
                          , Size
                          , Equal
                          >
{
  typedef d_ary_heap_indirect<typename ValueContainer::type, Arity, DistanceMap, IndexInHeapPropertyMap
                              , Better, ValueContainer, Size, Equal> Base;
  priority_queue(DistanceMap const& distance,
                 IndexInHeapPropertyMap const& indexInHeap = IndexInHeapPropertyMap(),
                 Better const& better = Better(),
                 Size containerReserve = 100,
                 Equal const& equal = Equal())
      : Base(distance, indexInHeap, better, containerReserve, equal)
  {}
  /// see d_ary_heap_indirect for public methods (push(val), pop(), Val const& top()), adjust_top()
  priority_queue(Size containerReserve = 0)
      : Base(DistanceMap(), IndexInHeapPropertyMap(), Better(), containerReserve, Equal())
  {}

};


}

#endif
