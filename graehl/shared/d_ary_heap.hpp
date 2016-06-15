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

  heap of indices (or pointers) where distance (priority, lowest = at top by
  default), location in heap, etc. are stored in external property maps (usually
  separate arrays).

  you can update(), or push_or_update() the priority unlike vanilla binary-heap w/
  std::pop_heap etc.

  by default, d=4-ary min-heap (faster than 2-ary depending on key size, due to cache locality)

  TODO: it might be worth trying priority+index together in heap data structure
  for even more locality (these are almost always used together when maintaining
  heap invariant by moving nodes up or down). location should still be external
  since it's less frequently updated

  'min-heap' means first distance is at top() - (smallest w/ the default, std::less)

  TODO: C++11, this heap move assign things you add to it (TODO: investigate
  template / std::forward to support both move and non-?)
 */

#ifndef GRAEHL_SHARED__D_ARY_HEAP_HPP
#define GRAEHL_SHARED__D_ARY_HEAP_HPP
#pragma once

#include <graehl/shared/cpp11.hpp>
#ifndef GRAEHL_DEBUG_D_ARY_HEAP
#define GRAEHL_DEBUG_D_ARY_HEAP 0
#endif
#if GRAEHL_DEBUG_D_ARY_HEAP
#include <graehl/shared/ifdbg.hpp>
#include <graehl/shared/show.hpp>
#define DDARY(x) x
DECLARE_DBG_LEVEL(DDARY)
#else
#ifndef EIFDBG
#define EIFDBG(ch, l, e)
#endif
#define DDARY(x)
#endif

#include <boost/property_map/property_map.hpp>
#include <boost/smart_ptr/shared_array.hpp>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>

#ifdef _MSC_VER
#undef min
#endif

#define GRAEHL_D_ARY_APPEND_ALWAYS_PUSH 1
// heapify (0) is untested.  0 means switch between push and heapify depending
// on size (cache effects, existing items vs. # appended ones)

#define GRAEHL_D_ARY_TRACK_OUT_OF_HEAP 0
// set to 1 to slightly speed up contains(), if you are willing to preinit loc
// map to GRAEHL_D_ARY_HEAP_NULL_INDEX-but should work fine with 0 because we avoid
// false positives by checking key at that location.

#define GRAEHL_D_ARY_VERIFY_HEAP 0
#if !defined(NDEBUG) && GRAEHL_D_ARY_VERIFY_HEAP
#undef GRAEHL_D_ARY_VERIFY_HEAP
#define GRAEHL_D_ARY_VERIFY_HEAP 0
// VERY slow if enabled
#endif

#undef GRAEHL_D_ARY_HEAP_NULL_INDEX
#define GRAEHL_D_ARY_HEAP_NULL_INDEX (-1)
// you may init location map to this for GRAEHL_D_ARY_TRACK_OUT_OF_HEAP (optional), or for push_or_update


#ifndef GRAEHL_D_ARY_HEAP_CACHE_ALIGN_ROOT
#define GRAEHL_D_ARY_HEAP_CACHE_ALIGN_ROOT 0
#endif

#if GRAEHL_D_ARY_HEAP_CACHE_ALIGN_ROOT
#include <graehl/shared/aligned_allocator.hpp>
#define GRAEHL_D_ARY_DEFAULT_CONTAINER(T, ARITY) std::vector<T, graehl::aligned_allocator<T, ARITY>>
#else
#define GRAEHL_D_ARY_DEFAULT_CONTAINER(T, ARITY) std::vector<T>
#endif

namespace graehl {


template <class Key, class Index = unsigned>
struct no_index_in_heap {};
template <class Key, class Index>
Index get(no_index_in_heap<Key, Index> const&, Key const& k) {
  std::abort();
  return 0;
}
template <class Key, class Index, class Index2>
void put(no_index_in_heap<Key, Index> const&, Key const&, Index2 i) {
  return;
}

template <class Key, class Distance>
struct deref_distance {};
template <class Key, class Distance>
Distance get(deref_distance<Key, Distance> const&, Key const& k) {
  return *k;
}
template <class Key, class Distance>
void put(deref_distance<Key, Distance> const&, Key const&, Distance t) {
  std::abort();
}

template <class K>
struct identity_distance {
  typedef K key_type;
  typedef K value_type;
  typedef K const& reference;
  typedef boost::readable_property_map_tag category;
  K const& operator[](K const& key) const { return key; }
};
template <class K>
K const& get(identity_distance<K>, K const& key) {
  return key;
}
}

namespace boost {
template <class PMap>
struct property_traits;
template <class Key, class Index>
struct property_traits<graehl::no_index_in_heap<Key, Index>> {
  typedef boost::writable_property_map_tag category;
  typedef Key key_type;
  typedef Index value_type;
};
template <class Key, class Distance>
struct property_traits<graehl::deref_distance<Key, Distance>> {
  typedef boost::writable_property_map_tag category;
  typedef Key key_type;
  typedef Distance value_type;
};
}

namespace graehl {
static const std::size_t OPTIMAL_HEAP_ARITY
    = 2;  // boost docs claimed 4 was optimal for dijkstra. TODO: validate that

/* adapted from boost/graph/detail/d_ary_heap.hpp

  local modifications:

  clear, heapify, append range/container, Size type template arg, reserve constructor arg

  hole+move rather than swap.  note: swap would be more efficient for heavyweight keys, until move ctors exist

  don't set locmap to -1 when removing from heap (waste of time)

  indices start at 0, not 1:
  // unlike arity=2 case, you don't gain anything by having indices start at 1, with 0-based child indices
  // root @1, A=2, children indices m= {0,1}: parent(i)=i/2, child(i, m)=2*i+m
  // root @0: parent(i)=(i-1)/A child(i, n)=i*A+n+1-can't improve on this except child(i, m)=i*A+m
  (integer division, a/b=floor(a/b), so (i-1)/A = ceil(i/A)-1, or greatest int less than (i/A))

  actually, no need to adjust child index, since child is called only once and inline

  e.g. for A=3 gorn address in tree -> index

  () = root -> 0
  (1) -> 1
  (2) -> 2
  (3) (A) -> 3
  (1,1) -> (1*A+1) = 4
  (1,2) -> (1*A+2) = 5
  (1,3) -> (1*A+3) = 6
  (2,1) -> (2*A+1) = 7
  etc.

//TODO: block-align siblings!  assume data[0] is 16 or 32-byte aligned ... then we want root @ index
(blocksize-1).  see http://www.lamarca.org/anthony/pubs/heaps.pdf pg8.  for pow2(e.g. 4)-ary heap, it may be
reasonable to  use root @index A-1.  however, suppose the key size is not padded to a power of 2 (e.g. 12
bytes), then we would need internal gaps at times.  would want to use compile const template based inlineable
alignment math for this?  possibly use a container like vector that lets you specify padding relative to some
address multiple for v[0].

 optimal D: see http://www.lamarca.org/anthony/pubs/heaps.pdf pg 9.  depedns on relative cost of swap,
compare, but in all cases except swap=free, 2 is worse than 3-4.  for expensive swap (3x compare), 4 still as
good as 5.  so just use 4.  boost benchmarking djikstra agrees; 4 is best.

 cache-aligned 4-heap speedup over regular 2-heap is 10-80% (for huge heaps, the speedup is more)

 splay/skew heaps are worse than 2heap or aligned 4heap in practice.

 //TODO: switch from heapify (Floyd's method) to repeated push past some size limit (in bytes) due to cache
effect -
 #define GRAEHL_D_ARY_BYTES_OUT_OF_CACHE 0x1000000

 //TODO: assuming locmap is an lvalue pmap, we can be more efficient.  on the other hand, if it's an intrusive
property map to an interned mutable object, there's no difference in performance, and that's what i'm going to
do in my first uses.  plus, if keys are indices and the map is a vector, it's barely any overhead.

 */

//
//=======================================================================
// Copyright 2009 Trustees of Indiana University
// Authors: Jeremiah J. Willcock, Andrew Lumsdaine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
//

#include <boost/shared_array.hpp>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <utility>
#include <vector>


// D-ary heap using an indirect compare operator (use identity_property_map
// as DistanceMap to get a direct compare operator).  This heap appears to be
// commonly used for Dijkstra's algorithm for its good practical performance
// on some platforms; asymptotically, it's not optimal; it has an O(lg N) decrease-key
// operation, which is (amortized) constant time on a relaxed heap or fibonacci heap.  The
// implementation is mostly based on the binary heap page on Wikipedia and
// online sources that state that the operations are the same for d-ary
// heaps.  This code is not based on the old Boost d-ary heap code.
//
// - d_ary_heap_indirect is a model of UpdatableQueue as is needed for
//   dijkstra_shortest_paths.
//
// - Value must model Assignable.
// - Arity must be at least 2 (optimal value appears to be 4, both in my and
//   third-party experiments).
// - IndexInHeapMap must be a ReadWritePropertyMap from Value to
//   Container::size_type (to store the index of each stored value within the
//   heap for decrease-key aka update).
// - DistanceMap must be a ReadablePropertyMap from Value to something
//   (typedef'ed as distance_type).
// - Compare must be a BinaryPredicate used as a less-than operator on
//   distance_type.
// - Container must be a random-access, contiguous container (in practice,
//   the operations used probably require that it is std::vector<Value>).
//
template <class Value, std::size_t Arity, class DistanceMap = identity_distance<Value>,
          class IndexInHeapPropertyMap = no_index_in_heap<Value, unsigned>,
          class Better = std::less<typename boost::property_traits<DistanceMap>::value_type>,
          class Container = GRAEHL_D_ARY_DEFAULT_CONTAINER(Value, Arity), class Size = unsigned,
          class Equal = std::equal_to<Value>>
class d_ary_heap_indirect {

 public:
#if GRAEHL_CPP11
  // TODO: use perfect forwarding of universal reftype args
  typedef Value&& MoveableValueRef;
#define GRAEHL_D_ARY_FORWARD_REF(x) std::forward<Value>(x)
// TODO: replace more (MoveableValueRef)x by this
#else
  typedef Value const& MoveableValueRef;
#define GRAEHL_D_ARY_FORWARD_REF(x) x
#endif
  typedef Container container_type;
  typedef Size size_type;
  typedef Value value_type;
  typedef typename Container::const_iterator const_iterator;
  typedef const_iterator iterator;
  // The distances being compared using better and that are stored in the
  // distance map
  typedef typename boost::property_traits<DistanceMap>::value_type distance_type;
  d_ary_heap_indirect(DistanceMap const& distance = DistanceMap(),
                      IndexInHeapPropertyMap const& index_in_heap = IndexInHeapPropertyMap(),
                      Better const& better = Better(), size_type container_reserve = 1024,
                      Equal const& equal = Equal())
      : better(better), data(), distance(distance), index_in_heap(index_in_heap), equal(equal) {
    if (container_reserve) data.reserve(container_reserve);
  }
  /* Implicit copy constructor */
  /* Implicit assignment operator */

  template <class C>
  void append_heapify(C const& c) {
    data.reserve(data.size() + c.size());
    append_heapify(c.begin(), c.end());
  }

  template <class I>
  void append_heapify(I begin, I end) {
    std::size_t s = data.size();
    data.insert(data.end(), begin, end);
    set_index_in_heap(s);  // allows contains() and heapify() to function properly
    heapify();
  }

  template <class C>
  void append_push(C const& c) {
    data.reserve(data.size() + c.size());
    append_push(c.begin(), c.end());
  }

  // past some threshold, this should be faster than append_heapify.  also, if there are many existing
  // elements it will be faster.
  template <class I>
  void append_push(I begin, I end) {
    for (; begin != end; ++begin) push(*begin);
  }

  template <class C>
  void append(C const& c) {
    if (GRAEHL_D_ARY_APPEND_ALWAYS_PUSH || data.size() >= c.size() / 2)
      append_push(c);
    else
      append_heapify(c);
  }

  // past some threshold, this should be faster than append_heapify.  also, if there are many existing
  // elements it will be faster.
  template <class I>
  void append(I begin, I end) {
    if (GRAEHL_D_ARY_APPEND_ALWAYS_PUSH || data.size() >= 0x10000)
      append_push(begin, end);
    else
      append_heapify(begin, end);
  }

  /**
     doesn't maintain heap property for you. so unless you added in sorted order, you must call
     finish_adding() after
  */
  template <class V>
  void add_unsorted(V const& v) {
    put(index_in_heap, v, data.size());  // allows contains() and heapify() to function properly
    data.push_back(v);
  }

#if GRAEHL_CPP11
  template <class... Args>
  void emplace_unsorted(Args&&... args) {
    Size i = data.size();
    data.emplace_back(std::forward<Args>(args)...);
    put(index_in_heap, data.back(), i);
  }
#else
  template <class V>
  void emplace_unsorted(V const& v) {
    Size i = data.size();
    data.push_back(v);
    put(index_in_heap, data.back(), i);
  }
#endif

  /// call after all add/emplace_unsorted done, unless you already
  void finish_adding() { heapify(); }

  // for debugging, please
  size_type loc(Value const& v) const { return get(index_in_heap, v); }

  // call heapify yourself after.
  // could allow mutation of data directly, e.g. push_back 1 at a time - but then they could forget to
  // heapify()

  // from bottom of heap tree up, turn that subtree into a heap by adjusting the root down
  // for n=size, array elements indexed by floor(n/2) + 1, floor(n/2) + 2, ... , n are all leaves for the
  // tree, thus each is an one-element heap already
  // warning: this is many fewer instructions but, at some point (when heap doesn't fit in Lx cache) it will
  // become slower than repeated push().
  void set_index_in_heap(std::size_t i = 0) {
    for (std::size_t e = data.size(); i < e; ++i) {
      // cppcheck-suppress unusedScopedObject
      put(index_in_heap, data[i], i);
    }
  }

  /*
The basic building block of "heapify" is a simple procedure which I'd
call "preserve_heap_property_down". It looks at a node that is not a leaf, and among
that non-leaf node and its two children, which is largest. Then it
swap that largest node to the top, so that the node becomes the
largest among the three---satisfy the "heap property". Clearly,
constant time.

To heapify the whole tree, we will heapify it from the bottom (so
"bottom up"). Heapifying a node at the "bottom" (i.e., next to a
leaf) is trivial; heapifying a node that is not bottom might cause the
heap property of nodes below to be violated. In that case we will
have to heapify the node below as well, which might cause a node one
more level below to violate heap property. So to heapify a node at
level n, we might need to call preserve_heap_property_down (height-1) times.

Now we show that a whole run of heapify of a tree with n=2^k-1 nodes
will never call preserve_heap_property_down more than n times. We do it by finding a
way to charge the calls to preserve_heap_property_down so that each node is never
charged more than once. Note that we simplify things by always
considering full binary trees (otherwise, the claim has to be a bit
more complicated).

Let's consider the bottom layer. To heapify a node with two children,
we need one call to preserve_heap_property_down. It is charged to left leaf. So we
have this:

After heapify O
/ \
X O

where O shows a node that is not charged yet, and X show a node which
is already charged. Once all the next-to-bottom nodes have been
heapified, we start to heapify the next-to-next-to-bottom nodes.
Before heapifying a node there, we have

Before heapify O
_/ \_
O O
/ \ / \
X O X O

To heapify this node, we might need two calls to preserve_heap_property_down. We
charge these two calls to the two uncharged nodes of the left subtree.
So we get:

After heapify O
_/ \_
X O
/ \ / \
X X X O

So none of the nodes is charged more than once, and we still have some
left for the next level, where before heapify the picture is:

Before heapify O
____/ \____
O O
_/ \_ _/ \_
X O X O
/ \ / \ / \ / \
X X X O X X X O

Heapifying at this level requires at most 3 calls to preserve_heap_property_down,
which are charged again to the left branch. So after that we get

After heapify O
____/ \____
X O
_/ \_ _/ \_
X X X O
/ \ / \ / \ / \
X X X X X X X O

We note a pattern: the path towards the right-most leaf is never
charged. When heapifying a level, one of the branches will always
have enough uncharged nodes to pay for the "expensive" heapify at the
top, while the other branch will still be uncharged to keep the
pattern. So this pattern is maintained until the end of the heapify
procedure, making the number of steps to be at most n-k = 2^k-k-1.
This is definitely linear to n.
   */
  void heapify() {
    EIFDBG(DDARY, 1, SHOWM1(DDARY, "heapify", data.size()));
    for (size_type i = parent(data.size());
         i > 0;) {  // starting from parent of last node, ending at first child of root (i==1)
      --i;
      EIFDBG(DDARY, 2, SHOWM1(DDARY, "heapify", i));
      preserve_heap_property_down(i);
    }
    verify_heap();
  }

  void reserve(size_type s) { data.reserve(s); }

  size_type size() const { return data.size(); }

  bool empty() const { return data.empty(); }

  const_iterator begin() const { return data.begin(); }

  const_iterator end() const { return data.end(); }

  void clear() {
#if GRAEHL_D_ARY_TRACK_OUT_OF_HEAP
    using boost::put;
    for (typename Container::iterator i = data.begin(), e = data.end(); i != e; ++i)
      put(index_in_heap, *i, (size_type)GRAEHL_D_ARY_HEAP_NULL_INDEX);
#endif
    data.clear();
  }

/**
   you must have put v's distance in the DistanceMap before pushing
*/
#if GRAEHL_CPP11
  template <class Univ>
  void push(Univ&& v)
#else
  void push(MoveableValueRef v)
#endif
  {
    size_type index = data.size();
    if (index) {
#if GRAEHL_CPP11
      data.emplace_back();
#else
      data.push_back(Value());  // (hoping default construct is cheap, construct-copy inline)
#endif
      preserve_heap_property_up(GRAEHL_D_ARY_FORWARD_REF(v), index, get(distance, v));
      // we don't have to recopy v, or init index_in_heap
      verify_heap();
    } else {
      put(index_in_heap, v, 0);
#if GRAEHL_CPP11
      data.emplace_back(GRAEHL_D_ARY_FORWARD_REF(v));
#else
      data.push_back(v);  // (hoping default construct is cheap, construct-copy inline)
#endif
    }
  }

  Value& top() { return data[0]; }

  Value const& top() const { return data[0]; }

  Value& front() { return data[0]; }

  Value const& front() const { return data[0]; }

  /**
     as with top(), take care not to invalidate heap property or item<->location map
  */
  Value& operator[](size_type i) { return data[i]; }

  Value const& operator[](size_type i) const { return data[i]; }

  void pop() {
    using boost::put;
    if (GRAEHL_D_ARY_TRACK_OUT_OF_HEAP) put(index_in_heap, data[0], (size_type)GRAEHL_D_ARY_HEAP_NULL_INDEX);
    size_type sz = data.size();
    if (sz == 1)
      data.pop_back();
    else {
      put(index_in_heap, data[0] = (MoveableValueRef)data.back(), 0);
      data.pop_back();
      preserve_heap_property_down();
      verify_heap();
    }
  }

// This function assumes the key has been improved
// (distance has become smaller, so it may need to rise toward top().
// i.e. decrease-key in a min-heap
#if GRAEHL_CPP11
  template <class Univ>
  void update(Univ&& v)
#else
  void update(MoveableValueRef v)
#endif
  {
    using boost::get;
    size_type index = get(index_in_heap, v);
    assert(index <= data.size());
    preserve_heap_property_up(GRAEHL_D_ARY_FORWARD_REF(v), index, get(distance, v));
    verify_heap();
  }

  distance_type best(distance_type null = 0) const { return empty() ? null : get(distance, data[0]); }
  distance_type second_best(distance_type null = 0) const {
    if (data.size() < 2) return null;
    int m = std::min(data.size(), Arity + 1);
    distance_type b = get(distance, data[1]);
    for (int i = 2; i < m; ++i) {
      distance_type d = get(distance, data[i]);
      if (better(d, b)) b = d;
    }
    return b;
  }

#ifdef __clang__
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif

  bool contains(Value const& v, size_type i) const {
    if (GRAEHL_D_ARY_TRACK_OUT_OF_HEAP) return i != (size_type)GRAEHL_D_ARY_HEAP_NULL_INDEX;
    size_type sz = data.size();
    EIFDBG(DDARY, 2, SHOWM2(DDARY, "d_ary_heap contains", i, data.size()));
    return i >= 0 && i < sz && equal(v, data[i]);
    // note: size_type may be signed (don't recommend it, though) - thus i>=0
    // check to catch uninit. data
  }

  bool contains(Value const& v) const {
    using boost::get;
    return contains(v, get(index_in_heap, v));
  }

/** insert if not present, else update (update means *improve* (move closer to top) only) as in best-first
 * search*/
#if GRAEHL_CPP11
  template <class Univ>
  void push_or_update(Univ&& v)
#else
  void push_or_update(Value const& v)
#endif
  {
    using boost::get;
    size_type index = get(index_in_heap, v);
    if (contains(v, index))
      preserve_heap_property_up(GRAEHL_D_ARY_FORWARD_REF(v), index, get(distance, v));
    else
      push(GRAEHL_D_ARY_FORWARD_REF(v));
  }

 private:
  Better better;
  Container data;
  DistanceMap distance;
  IndexInHeapPropertyMap index_in_heap;
  Equal equal;

  // Get the parent of a given node in the heap
  static inline size_type parent(size_type index) { return (index - 1) / Arity; }

  // Get the child_idx'th child of a given node; 0 <= child_idx < Arity
  static inline size_type child(size_type index, std::size_t child_idx) {
    return index * Arity + child_idx + 1;
  }

  // Swap two elements in the heap by index, updating index_in_heap
  void swap_heap_elements(size_type index_a, size_type index_b) {
    assert(index_a != index_b);
    Value& willb = data[index_a];
    Value& willa = data[index_b];
    using std::swap;
    swap(willb, willa);
    put(index_in_heap, willa, index_a);
    put(index_in_heap, willb, index_b);
  }

  void move_heap_element(MoveableValueRef v, size_type ito) {
    using boost::put;
    put(index_in_heap, data[ito] = (MoveableValueRef)v, ito);
  }

  // Verify that the array forms a heap; commented out by default
  void verify_heap() const {
// This is a very expensive test so it should be disabled even when
// NDEBUG is not defined
#if GRAEHL_D_ARY_VERIFY_HEAP
    using boost::get;
    for (size_t i = 1; i < data.size(); ++i) {
      if (better(get(distance, data[i]), get(distance, data[parent(i)]))) {
        assert(!"Element is smaller than its parent");
      }
      if (get(index_in_heap, data[i]) != i) {
        assert(!"Element is where its index_in_heap doesn't say it is.");
      }
    }
#endif
  }

  // we have a copy of the key, so we don't need to do that stupid find # of levels to move then move.  we act
  // as though data[index]=currently_being_moved, but in fact it's an uninitialized "hole", which we fill at
  // the very end
  void preserve_heap_property_up(MoveableValueRef currently_being_moved, size_type index) {
    using boost::get;
    preserve_heap_property_up(GRAEHL_D_ARY_FORWARD_REF(currently_being_moved), index,
                              get(distance, currently_being_moved));
  }

  /**
     disabled because distance map may not be writable. would need traits to enable
  */
  /*
  void preserve_heap_property_up_set_dist(MoveableValueRef currently_being_moved, distance_type
  dbetter) {
    using boost::get;
    using boost::put;
    put(distance, currently_being_moved, dbetter);
    preserve_heap_property_up(currently_being_moved, get(index_in_heap, currently_being_moved), dbetter);
    verify_heap();
  }
  */

  /// put(index_in_heap, data[index] = currently_being_moved, index); preserve_heap_property_up(index);
  void preserve_heap_property_up(MoveableValueRef move_to_index_first, size_type index,
                                 distance_type move_to_index_first_dist) {
    using boost::put;
    using boost::get;
    assert(get(distance, move_to_index_first) == move_to_index_first_dist);
    for (;;) {
      if (index == 0) break;  // Stop at root - but still need to assign move_to_index_first to posn 0
      size_type parent_index = parent(index);
      Value& parent_value = data[parent_index];
      if (better(move_to_index_first_dist, get(distance, parent_value))) {
        put(index_in_heap, data[index] = (MoveableValueRef)parent_value, index);
        index = parent_index;
      } else {
        break;  // Heap property satisfied
      }
    }
    // finish "swap chain" by filling hole w/ move_to_index_first
    put(index_in_heap, data[index] = (MoveableValueRef)move_to_index_first, index);
  }

  // Starting at a node, move up the tree swapping elements to preserve the
  // heap property.  doesn't actually use swap; uses hole
  void preserve_heap_property_up(size_type index) {
    using boost::get;
    if (index == 0) return;  // Do nothing on root
    Value& vali = data[index];
    size_type parent_index = parent(index);
    distance_type disti = get(distance, vali);
    Value& valparent = data[parent_index];
    if (better(disti, get(distance, valparent))) {
      Value copyi(vali);
      put(index_in_heap, data[index] = (MoveableValueRef)valparent, index);
      preserve_heap_property_up((MoveableValueRef)copyi, parent_index, disti);
    }
    return;
    verify_heap();
  }


  /// \return 0 if currently_being_moved is better than all children of parent_index else position of child
  /// that should move to parent_index (best child). heap_size should == data.size() (caching)
  size_type which_child_down(size_type parent_index, distance_type parent_dist, Value* heap_ptr,
                             size_type heap_size) {
    size_type first_child_index = child(parent_index, 0);
    size_type smallest_child_index = 0;
    distance_type d;
    for (size_type i = first_child_index, e = std::min(first_child_index + Arity, heap_size); i < e; ++i)
      if (better(d = get(distance, heap_ptr[i]), parent_dist)) {
        smallest_child_index = i;
        parent_dist = d;
      }
    return smallest_child_index;
  }


#undef GRAEHL_D_ARY_MAYBE_IMPROVE_CHILD_I
#define GRAEHL_D_ARY_MAYBE_IMPROVE_CHILD_I                   \
  do {                                                       \
    distance_type i_dist = get(distance, child_base_ptr[i]); \
    if (better(i_dist, smallest_child_dist)) {               \
      smallest_child_index = i;                              \
      smallest_child_dist = i_dist;                          \
    }                                                        \
  } while (0)
/** From the root, swap elements (each one with its smallest child) if there
   are any parent-child pairs that violate the heap property.  v is placed at
   data[i], but then pushed down (note: data[i] won't be read explicitly; it
   will instead be overwritten by percolation).  this also means that v must
   be a copy of data[i] if it was already at i.  e.g. v=data.back(), i=0,
   sz=data.size()-1 for pop(), implicitly swapping data[i], data.back(), and
   doing data.pop_back(), then adjusting from 0 down w/ swaps.  updates
   index_in_heap for v. on first call currently_being_moved must have been at
   index.  .
*/
#if GRAEHL_CPP11
  template <class Univ>
#endif
  void preserve_heap_property_down(
#if GRAEHL_CPP11
      Univ&& move_to_index_first,
#else
      MoveableValueRef move_to_index_first,
#endif
      size_type index, size_type heap_size) {
    using boost::put;
    //// hole at index - move_to_index_first to be put here when we find the final hole spot
    EIFDBG(DDARY, 4, SHOWM3(DDARY, "preserve_heap_property_down impl", index, move_to_index_first, heap_size));
    using boost::get;
    distance_type move_down_dist = get(distance, move_to_index_first);
    Value* data_ptr = &data[0];
    for (;;) {
      size_type first_child_index = child(index, 0);
      if (first_child_index >= heap_size) {
        put(index_in_heap, data[index] = (MoveableValueRef)move_to_index_first, index);
        break;
      }
      Value* child_base_ptr
          = data_ptr + first_child_index;  // using index of first_child_index+smallest_child_index because we
      // hope optimizer will be smart enough to const-unroll a loop below
      // if we do this.  i think the optimizer would have gotten it even
      // without our help (i.e. store root-relative index)

      // begin find best child index/distance
      size_type smallest_child_index
          = 0;  // don't add to base first_child_index every time we update which is smallest.
      distance_type smallest_child_dist = get(distance, child_base_ptr[smallest_child_index]);
      if (first_child_index + Arity <= heap_size) {
        // avoid repeated heap_size boundcheck (should test if this is really a speedup - instruction cache
        // tradeoff - could use upperbound = min(Arity, heap_size-first_child_index) instead.  but this
        // optimizes to a fixed number of iterations (compile time known) so probably worth it
        for (size_type i = 1; i < Arity; ++i) {
          GRAEHL_D_ARY_MAYBE_IMPROVE_CHILD_I;
        }
      } else {
        for (size_type i = 1, e = heap_size - first_child_index; i < e; ++i) {
          GRAEHL_D_ARY_MAYBE_IMPROVE_CHILD_I;
        }
      }
      // end: know best child

      if (better(smallest_child_dist, move_down_dist)) {
        put(index_in_heap, data[index] = (MoveableValueRef)child_base_ptr[smallest_child_index], index);
        // instead of swapping, move.
        // move_heap_element((MoveableValueRef)child_base_ptr[smallest_child_index], index);  // move up
        index = first_child_index + smallest_child_index;  // descend - hole is now here
      } else {
        put(index_in_heap, data[index] = (MoveableValueRef)move_to_index_first, index);
        break;
        // move_heap_element((MoveableValueRef)move_to_index_first, index);  // finish "swap chain" by
        // filling hole
      }
    }
    verify_heap();
  }

  void preserve_heap_property_down(size_type i) {
    EIFDBG(DDARY, 3, SHOWM3(DDARY, "preserve_heap_property_down", i, data[i], data.size()));
    Value copyi(data[i]);  // TODO: we should only copy if heap property isn't already present. or give up and
    // just use swap for this (see usage from heapify)
    preserve_heap_property_down((MoveableValueRef)copyi, i, data.size());
  }

 public:
  /**
     if you modify top() and might need to move element down since it's no longer the least.
  */
  void adjust_top() { this->preserve_heap_property_down(); }

  // moves what's at root downwards if needed
  void preserve_heap_property_down() {
    assert(!data.empty());
    Value* v0 = &data[0];
    distance_type d0 = get(distance, *v0);
    size_type n = data.size();
    if (n > Arity) {
      size_type swapi = 0;
      for (size_type i = 1; i <= Arity; ++i) {
        distance_type di = get(distance, v0[i]);
        if (better(di, d0)) {
          d0 = di;
          swapi = i;
        }
      }
      if (swapi) {
        Value copy0(*v0);
        using boost::put;
        Value& vs = v0[swapi];
        put(index_in_heap, data[0] = (MoveableValueRef)vs, 0);
        preserve_heap_property_down((MoveableValueRef)copy0, swapi, n);
      }
    } else if (n > 1) {
      size_type swapi = 0;
      for (size_type i = 1; i < n; ++i) {
        distance_type di = get(distance, v0[i]);
        if (better(di, d0)) {
          d0 = di;
          swapi = i;
        }
      }
      if (swapi) swap_heap_elements(swapi, 0);
    }
    verify_heap();
  }
};


}

#endif
