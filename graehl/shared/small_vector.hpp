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
#ifndef GRAEHL_SHARED___SMALL_VECTOR
#define GRAEHL_SHARED___SMALL_VECTOR
#pragma once
#include <graehl/shared/cpp11.hpp>
#if GRAEHL_CPP11
#include <initializer_list>
#endif

/** \file

    space and cache-efficient small vectors (like std::vector) for POD-like (can
    memcpy to copy) d_. if you want constructors use std::vector.

    memcmp comparable isn't necessary - we use ==, <.

    for small size, elements are stored without indirection on the stack. over
    that size, they're stored on the heap as with a regular std::vector. specify
    the maximum 'small size' as a template constant. also specify an alternative
    to std::size_t (e.g. uint32_t or uint16_t) for element indices and sizes.

    offers same iterator invalidations as std::vector with one exception:
    erasing will also invalidate (when size transitions from >kMaxInlineSize to
    <=kMaxInlineSize). further, reserve won't prevent iterator invalidations on
    adding elements, unless size is already >kMaxInlineSize

    vectors indexed by size_type SIZE (if your element type T is small, then it
    pays to make SIZE small also). in debug mode you'll get assertions if you
    ever reach max SIZE. the default size type is 16-bit unsigned. so you only
    get 65535 elements max. in debug mode, the allowed size is cut in half
    intentionally so you have early warning.

    stores small element (<=kMaxInlineSize items) vectors inline.  recommend
    kMaxInlineSize so that kMaxInlineSize*sizeof(T)+2*sizeof(SIZE) is a
    multiple of 8, somewhere from 8-32 bytes.  or choose the speed/memory
    tradeoff empirically

    you can expect 8-byte alignment of pointers:
    http://www.x86-64.org/documentation/abi.pdf and i put the size int first
    because otherwise it wouldn't be in the same place for both large and small
    layout (without pulling it outside the union entirely, which would lose the
    ability to have size/capacity next to each other for large format.

    REQUIRES that T is POD (specifically - it's part of a union, so must be. all
    we really require is that memmove/memcpy is allowed rather than copy
    construct + destroy old. we don't call destructor on T.
*/

// don't change to 0 unless you add ~T() and copy/move calls (many will be
// needed)
#define GRAEHL_SMALL_VECTOR_POD_ONLY 1

#include <cstdlib>

/**
   if 1, complete valgrind safety at the cost of speed. if 0, code is still
   correct but gives spurious use of uninitialized value on copying inlined
   vectors
*/

#ifndef GRAEHL_VALGRIND
#ifdef NDEBUG
#define GRAEHL_VALGRIND 0
#else
#define GRAEHL_VALGRIND 1
#endif
// define as 0 and get faster copying from small->heap vector (may copy uninitialized elements)
#endif

#ifndef GRAEHL_SMALL_VECTOR_SERIALIZE_VERSION
/**
   enables item version info for the contained element type. one would hope that
   for primtive types there would be no wasteful versioning, but it's
   understandable that you'd want it for pod structs (that get a member added
   later)

   for vector<primitive> compatability with serialization via vector, this is
   must be 0 for binary archives, but must be 1 for text archives. it would be
   nice to figure out what traits need to be enabled so that it can be
   compatible with both

   see dicussion at:

   http://stackoverflow.com/questions/13655704/boostserialize-an-array-like-type-exactly-like-stdvector-without-copying
*/
#define GRAEHL_SMALL_VECTOR_SERIALIZE_VERSION 0
#endif
#if GRAEHL_SMALL_VECTOR_SERIALIZE_VERSION
#include <boost/serialization/item_version_type.hpp>
#include <boost/serialization/version.hpp>
#endif

#define GRAEHL_BOOST_SERIALIZATION_NVP(v) BOOST_SERIALIZATION_NVP(v)
#define GRAEHL_BOOST_SERIALIZATION_NVP_VERSION(v) BOOST_SERIALIZATION_NVP(v)

#include <boost/assign/list_inserter.hpp>
#include <boost/cstdint.hpp>
#include <boost/functional/hash.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/collection_size_type.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/wrapper.hpp>
#include <graehl/shared/good_alloc_size.hpp>
#include <graehl/shared/swap_pod.hpp>
#include <graehl/shared/word_spacer.hpp>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <iterator>
#include <new>
#include <vector>

namespace graehl {

template <class IterTraits>
struct enable_type {
  typedef void type;
};

enum { kDefaultMaxInlineSize = 3 };
typedef unsigned small_vector_default_size_type;

// TODO: for c++11 use gsl::span
template <class T, class Size = small_vector_default_size_type>
struct pod_array_ref {
  typedef Size size_type;
  typedef T const* const_iterator;
  typedef T* iterator;
  typedef T value_type;
  typedef T& reference;
  typedef T const& const_reference;
  Size sz;
  T* a;
  // cppcheck-suppress uninitMemberVar
  pod_array_ref() : sz() {}
  pod_array_ref(T* a, Size sz) : sz(sz), a(a) {}
  pod_array_ref(T* begin, T const* end) : sz(end - begin), a(begin) {}
  template <class Vec>
  explicit pod_array_ref(Vec const& vec) : sz((Size)vec.size()), a(&vec[0]) {
    assert(vec.size() == sz);
  }
  Size size() const { return sz; }
  Size length() const { return sz; }
  bool empty() const { return !sz; }
  T& front() {
    assert(sz);
    return a[0];
  }
  T& front() const {
    assert(sz);
    return a[0];
  }
  T& back() {
    assert(sz);
    return a[sz - 1];
  }
  T& back() const {
    assert(sz);
    return a[sz - 1];
  }

  T& operator[](Size i) {
    assert(i < sz);
    return a[i];
  }
  T const& operator[](Size i) const {
    assert(i < sz);
    return a[i];
  }
  T* begin() { return a; }
  T const* begin() const { return a; }
  T* data() { return a; }
  T const* data() const { return a; }
  T* end() { return a + sz; }
  T const* end() const { return a + sz; }
  operator T*() { return a; }
  operator T const*() const { return a; }
  template <class Vec>
  bool operator==(Vec const& vec) const {
    return sz == vec.size() && !std::memcmp(a, &vec[0], sz * sizeof(T));
  }
  template <class Vec>
  void operator=(Vec const& vec) {
    sz = vec.size();
    a == &vec[0];
  }
};

// recommend an odd number for kMaxInlineSize on 64-bit (check sizeof to see
// that you get the extra element for free)
template <class T, unsigned kMaxInlineSize = kDefaultMaxInlineSize, class Size = small_vector_default_size_type>
struct small_vector {
  bool operator==(pod_array_ref<T> const& o) const { return o == *this; }

  typedef small_vector Self;
  typedef Size size_type;
  typedef void memcpy_movable;
  /**
     may leak (if you don't free yourself)
  */
  void clear_nodestroy() { d_.stack.sz_ = 0; }

  T* push_back_uninitialized() {
    if (d_.stack.sz_ < kMaxInlineSize) {
      return &d_.stack.vals_[d_.stack.sz_++];
    } else {
      if (d_.stack.sz_ == kMaxInlineSize)
        copy_vals_to_ptr();
      else if (d_.stack.sz_ == d_.heap.capacity_)
        ensure_capacity_grow(d_.stack.sz_ + 1);
      return &d_.heap.begin_[d_.stack.sz_++];
    }
  }

  void push_back() { new (push_back_uninitialized()) T; }

#if GRAEHL_CPP11
  small_vector(std::initializer_list<T> const& l)
  {
    alloc(l.size());
    std::copy(l.begin(), l.end(), begin());
  }

  /// move
  small_vector(small_vector&& o) noexcept {
    std::memcpy(this, &o, sizeof(small_vector)); // NOLINT
    o.clear_nodestroy();
  }

  void init_range(small_vector const& o, size_type begin, size_type end) {
    const_iterator data = o.begin();
    init_range(data + begin, data + end);
  }

  /// move
  small_vector& operator=(small_vector&& o) noexcept {
    assert(&o != this);
    free();
    std::memcpy(this, &o, sizeof(small_vector)); // NOLINT
    o.clear_nodestroy();
    return *this;
  }

  /// varargs forwarded to ctor
  template <class... Args>
  void emplace_back(Args&&... args) {
    new (push_back_uninitialized()) T(std::forward<Args>(args)...);
  }
  void moveAssign(small_vector& o) {
    assert(&o != this);
    *this = std::move(o);
  }
#else
  void moveAssign(small_vector& o) {
    assert(&o != this);
    free();
    std::memcpy(this, &o, sizeof(small_vector)); // NOLINT
    o.clear_nodestroy();
  }

  void emplace_back() { new (push_back_uninitialized()) T; }

  template <class T0>
  void emplace_back(T0 const& a0) {
    new (push_back_uninitialized()) T(a0);
  }

  template <class T0, class T1>
  void emplace_back(T0 const& a0, T1 const& a1) {
    new (push_back_uninitialized()) T(a0, a1);
  }

  template <class T0, class T1, class T2>
  void emplace_back(T0 const& a0, T1 const& a1, T2 const& a2) {
    new (push_back_uninitialized()) T(a0, a1, a2);
  }

  template <class T0, class T1, class T2, class T3>
  void emplace_back(T0 const& a0, T1 const& a1, T2 const& a2, T3 const& a3) {
    new (push_back_uninitialized()) T(a0, a1, a2, a3);
  }
#endif
  // cppcheck-suppress unknownMacro
  BOOST_SERIALIZATION_SPLIT_MEMBER()
  template <class Archive>
  void save(Archive& ar, const unsigned) const {
    using namespace boost::serialization;
    collection_size_type const count(d_.stack.sz_);
    ar << GRAEHL_BOOST_SERIALIZATION_NVP(count);
#if GRAEHL_SMALL_VECTOR_SERIALIZE_VERSION
    item_version_type const item_version(version<T>::value);
    ar << GRAEHL_BOOST_SERIALIZATION_NVP_VERSION(item_version);  // for text archive only apparently?
// use_array_optimization only fires for
// binary archive?
#endif
    if (d_.stack.sz_) ar << GRAEHL_BOOST_SERIALIZATION_NVP(make_array(begin(), d_.stack.sz_));
  }
  template <class Archive>
  void load(Archive& ar, const unsigned) {
    using namespace boost::serialization;
    collection_size_type count;
    ar >> GRAEHL_BOOST_SERIALIZATION_NVP(count);
    resize((size_type)count);
    assert(count == d_.stack.sz_);
#if GRAEHL_SMALL_VECTOR_SERIALIZE_VERSION
    item_version_type version;
    ar >> GRAEHL_BOOST_SERIALIZATION_NVP_VERSION(version);
#endif
    if (d_.stack.sz_) ar >> GRAEHL_BOOST_SERIALIZATION_NVP(make_array(begin(), d_.stack.sz_));
  }

#if GRAEHL_CPP11
  constexpr small_vector() : d_{} {}
#else
  small_vector() { d_.stack.sz_ = 0; }
#endif
  /**
     default constructed T() * s. since T is probably POD, this means its pod
     members are default constructed (i.e. set to 0). If T isn't POD but can be memmoved, then you probably
     really wanted the constructor called.
  */
  explicit small_vector(size_type s) {
    assert(s <= kMaxSize);
    alloc(s);
    if (s <= kMaxInlineSize)
      for (size_type i = 0; i < s; ++i) new (&d_.stack.vals_[i]) T();
    else
      for (size_type i = 0; i < d_.stack.sz_; ++i) new (&d_.heap.begin_[i]) T();
  }

  small_vector(int s, T const& v) {
    assert(s >= 0);
    init((size_type)s, v);
  }

#ifndef SDL_32  // avoid signature collision
  small_vector(unsigned s, T const& v) { init((size_type)s, v); }
#endif

  small_vector(std::size_t s, T const& v) { init((size_type)s, v); }

  template <class Val>
  small_vector(int s, Val const& v) {
    assert(s >= 0);
    init((size_type)s, v);
  }

#ifndef SDL_32  // avoid signature collision
  template <class Val>
  small_vector(unsigned s, Val const& v) {
    init((size_type)s, v);
  }
#endif

  template <class Val>
  small_vector(std::size_t s, Val const& v) {
    init((size_type)s, v);
  }

  small_vector(T const* i, T const* end) { init_range(i, end); }

  small_vector(T const* i, size_type N) { init_range(i, N); }

  template <class Iter>
  small_vector(Iter const& i, Iter const& end, typename enable_type<typename Iter::value_type>::type* = 0)
  // couldn't SFINAE on std::iterator_traits<Iter> in gcc (for Iter=int)
  {
    init_range(i, end);
  }

  void init(size_type s, T const& v) {
    assert(s <= kMaxSize);
    alloc(s);
    if (s <= kMaxInlineSize) {
      for (size_type i = 0; i < s; ++i) d_.stack.vals_[i] = v;
    } else {
      for (size_type i = 0; i < d_.stack.sz_; ++i) d_.heap.begin_[i] = v;
    }
  }
  void init_range(T const* i, size_type s) {
    assert(s <= kMaxSize);
    alloc(s);
    memcpy_from(i);
  }
  void init_range(T const* i, T const* end) {
    size_type s = (size_type)(end - i);
    assert(s <= kMaxSize);
    alloc(s);
    memcpy_from(i);
  }
  template <class I>
  void init_range(I const& i, I const& end) {
    size_type s = (size_type)std::distance(i, end);
    assert(s <= kMaxSize);
    alloc(s);
    std::copy(i, end, begin());
  }

  /**
     copy ctor.
  */
  small_vector(small_vector const& o) {
    if (o.d_.stack.sz_ <= kMaxInlineSize) {
#if GRAEHL_VALGRIND
      d_.stack.sz_ = o.d_.stack.sz_;
      for (size_type i = 0; i < d_.stack.sz_; ++i) d_.stack.vals_[i] = o.d_.stack.vals_[i];
#else
      d_.stack = o.d_.stack;
#endif
      // TODO: valgrind for partial init array
    } else {
      d_.heap.capacity_ = d_.heap.sz_ = o.d_.heap.sz_;
      alloc_heap();
      memcpy_heap(o.d_.heap.begin_);
    }
  }

  template <class Alloc>
  small_vector(std::vector<T, Alloc> const& vec) {
    init(&*vec.begin(), &*vec.end());
  }

  typedef T const* const_iterator;
  typedef T* iterator;
  typedef T value_type;
  typedef T& reference;
  typedef T const& const_reference;
  static const size_type kMaxSize =
#ifdef NDEBUG
      (size_type)-1;
#else
      ((size_type)-1) / 2;
#endif
  typedef good_vector_size<T> Tarsize;
  enum { kTargetBiggerSz = kMaxInlineSize * 3 + 1 / 2 };
  enum { kTargetBiggerUnaligned = sizeof(T) * kTargetBiggerSz };
  enum {
    kTargetBiggerAligned = (kTargetBiggerUnaligned + ktarget_first_alloc_mask) & ~(size_type)ktarget_first_alloc_mask
  };
  static const size_type kInitHeapSize = Tarsize::ktarget_first_sz > kMaxInlineSize
                                             ? Tarsize::ktarget_first_sz
                                             : kTargetBiggerAligned / sizeof(T);
  T* begin() { return d_.stack.sz_ > kMaxInlineSize ? d_.heap.begin_ : d_.stack.vals_; }
  T const* begin() const { return const_cast<small_vector*>(this)->begin(); }
  T* data() { return begin(); }
  T const* data() const { return begin(); }

  T* end() { return begin() + d_.stack.sz_; }
  T const* end() const { return begin() + d_.stack.sz_; }
  typedef std::pair<T const*, T const*> const_slice_type;
  const_slice_type slice() const {
    return d_.stack.sz_ > kMaxInlineSize ? slice_type(d_.heap.begin_, d_.heap.begin_ + d_.stack.sz_)
                                         : slice_type(d_.stack.vals_, d_.stack.vals_ + d_.stack.sz_);
  }
  typedef std::pair<T*, T*> slice_type;
  slice_type slice() {
    return d_.stack.sz_ > kMaxInlineSize ? slice_type(d_.heap.begin_, d_.heap.begin_ + d_.stack.sz_)
                                         : slice_type(d_.stack.vals_, d_.stack.vals_ + d_.stack.sz_);
  }

  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
  const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }

  // TODO: test.  this invalidates more iterators than std::vector since resize may move from ptr to vals.
  T* erase(T* b) { return erase(b, b + 1); }

  void movelen(size_type to, size_type from, size_type len) {
    T* b = begin();
    std::memmove(b + to, b + from, len * sizeof(T));
  }

  // memmove instead of memcpy for case when someone erases middle of vector and
  // so source, destination ranges overlap
  static void movelen(T* to, T const* from, size_type len) { std::memmove(to, from, len * sizeof(T)); }

  void erase(size_type ibegin, size_type iend) {
    size_type sz = d_.stack.sz_;
    if (iend >= sz) {
      resize(ibegin);
    } else {
      size_type nafter = sz - iend;
      movelen(ibegin, iend, nafter);
      resize(ibegin + nafter);
    }
  }

  T* erase(T* b, T* e) {  // remove [b, e) and return pointer to element e
    T *tb = begin(), *te = end();
    size_type nbefore = (size_type)(b - tb);
    if (e == te) {
      resize(nbefore);
    } else {
      size_type nafter = (size_type)(te - e);
      movelen(b, e, nafter);
      resize(nbefore + nafter);
    }
    return begin() + nbefore;
  }
  void memcpy_to(T* p) { std::memcpy(p, begin(), d_.stack.sz_ * sizeof(T)); }
  void memcpy_heap(T const* from) {
    assert(d_.stack.sz_ <= kMaxSize);
    assert(d_.heap.capacity_ >= d_.stack.sz_);
    std::memcpy(d_.heap.begin_, from, d_.stack.sz_ * sizeof(T));
  }
  void memcpy_from(T const* from) {
    assert(d_.stack.sz_ <= kMaxSize);
    std::memcpy(this->begin(), from, d_.stack.sz_ * sizeof(T));
  }
  static inline void memcpy_n(T* to, T const* from, size_type n) { std::memcpy(to, from, n * sizeof(T)); }
  static inline void memmove_n(T* to, T const* from, size_type n) { std::memmove(to, from, n * sizeof(T)); }

  small_vector& operator=(small_vector const& o) {
    if (&o == this) return *this;
    if (d_.stack.sz_ <= kMaxInlineSize) {
      if (o.d_.stack.sz_ <= kMaxInlineSize) {
        d_.stack.sz_ = o.d_.stack.sz_;
        // unsigned instead of size_type because kMaxInlineSize is unsigned
        for (unsigned i = 0; i < kMaxInlineSize; ++i) d_.stack.vals_[i] = o.d_.stack.vals_[i];
      } else {
        d_.heap.capacity_ = d_.stack.sz_ = o.d_.stack.sz_;
        alloc_heap();
        memcpy_heap(o.d_.heap.begin_);
      }
    } else {
      if (o.d_.stack.sz_ <= kMaxInlineSize) {
        free_heap();
        d_.stack.sz_ = o.d_.stack.sz_;
        for (unsigned i = 0; i < d_.stack.sz_; ++i) d_.stack.vals_[i] = o.d_.stack.vals_[i];
      } else {
        if (d_.heap.capacity_ < o.d_.stack.sz_) {
          free_heap();
          d_.heap.capacity_ = o.d_.stack.sz_;
          alloc_heap();
        }
        d_.stack.sz_ = o.d_.stack.sz_;
        memcpy_heap(o.d_.heap.begin_);
      }
    }
    return *this;
  }

  ~small_vector() { free(); }

  /// null values can't be assigned to or from (will crash - TODO: fix w/o undue
  /// perf penalty? better customize densehashtable.h) but can be set/is/~small_vector
  void is_any_null() const { return d_.stack.sz_ >= (size_type)-2; }
  void is_null() const { return d_.stack.sz_ == (size_type)-1; }
  void is_null2() const { return d_.stack.sz_ == (size_type)-2; }
  void set_null() {
    d_.stack.sz_ = (size_type)-1;
    d_.heap.begin_ = 0;
  }
  void set_null2() {
    d_.stack.sz_ = (size_type)-2;
    d_.heap.begin_ = 0;
  }

  void clear() {
    free();
    d_.stack.sz_ = 0;
  }

  void assign(T const* array, size_type n) { memcpy_n(realloc(n), array, array + n); }

  void assign(T const* i, T const* end) {
    size_type n = (size_type)(end - i);
    memcpy_n(realloc(n), i, n);
  }
  void assign(T const* array, size_type begin, size_type end) {
    size_type n = end - begin;
    memcpy_n(realloc(n), array + begin, array + end);
  }
  template <class I>
  void assign(I i, I end) {
    T* o = realloc((size_type)std::distance(i, end));
    for (; i != end; ++i) *o++ = *i;
  }
  template <class Set>
  void assign(Set const& set) {
    assign(set.begin(), set.end());
  }
  template <class I>
  void assign_input(I i, I end) {
    clear();
    for (; i != end; ++i) push_back(*i);
  }
  void assign(size_type n, T const& v) {
    free();
    init(n, v);
  }
  template <class Vec>
  void assignTo(size_type to, Vec const& vec) {
    assignTo(to, vec.begin(), vec.size());
  }

  bool empty() const { return d_.stack.sz_ == 0; }
  size_type size() const { return d_.stack.sz_; }
  size_type length() const { return d_.stack.sz_; }
  size_type capacity() const { return d_.stack.sz_ > kMaxInlineSize ? d_.heap.capacity_ : d_.stack.sz_; }
  // our true capacity is never less than kMaxInlineSize, but since capacity talks about allocated space,
  // we may as well return 0; however, docs on std::vector::capacity imply capacity>=size.

  /// does not initialize w/ default ctor.
  void resize_up_unconstructed(size_type s) {  // like reserve, but because of capacity_ undef if
    // d_.stack.sz_<=kMaxInlineSize, must set d_.stack.sz_
    // immediately or we lose invariant
    assert(s >= d_.stack.sz_);
    if (s > kMaxInlineSize) realloc_big(s);
    d_.stack.sz_ = s;
  }
  void resize_up_big_unconstructed(size_type s) {
    assert(s > kMaxInlineSize);
    realloc_big(s);
    d_.stack.sz_ = s;
  }
  void resize_maybe_unconstructed(size_type s) {
    if (s < d_.stack.sz_)
      compact(s);
    else
      resize_up_big_unconstructed(s);
  }

  void reserve(size_type s) {
    if (d_.stack.sz_ > kMaxInlineSize) realloc_big(s);
  }
  static inline void construct_range(T* v, size_type begin, size_type end) {
    for (size_type i = begin; i < end; ++i) new (&v[i]) T();
  }

  static inline void construct_range(T* v, size_type begin, size_type end, T const& val) {
    for (size_type i = begin; i < end; ++i) new (&v[i]) T(val);
  }

  // aliasing warning: if append is within self, undefined result (because we invalidate iterators as we grow)
  /**
     append the range [i, end) without being able to forecast the size in advance:
  */
  template <class InputIter>
  void append_input(InputIter i, InputIter end) {
    if (d_.small.sz_ <= kMaxInlineSize) {
      for (; d_.small.sz_ < kMaxInlineSize; ++d_.small.sz_) {
        d_.small.vals_[d_.small.sz_] = *i;
        ++i;
        if (i == end) return;
      }
      copy_vals_to_ptr();
      d_.large.begin_[kMaxInlineSize] = *i;
      ++i;
      d_.small.sz_ = kMaxInlineSize + 1;
    }
    for (; i != end; ++i) push_back_large(*i);
  }

  template <class ForwardIter>
  void append(ForwardIter i, ForwardIter e) {
    size_type s = d_.stack.sz_;
    size_type addsz = (size_type)(e - i);
    resize_up_unconstructed(s + addsz);
    for (T *b = begin() + s; i < e; ++i, ++b) *b = *i;
  }

  void append(T const* i, size_type n) {
    size_type s = d_.stack.sz_;
    append_unconstructed(n);
    memcpy_n(begin() + s, i, n);
  }

  void append(T const* i, T const* end) {
    size_type s = d_.stack.sz_;
    size_type n = (size_type)(end - i);
    append_unconstructed(n);
    memcpy_n(begin() + s, i, n);
  }

  template <class Set>
  void append(Set const& set) {
    append(set.begin(), set.end());
  }

  void append(small_vector const& set) { append(set.begin(), set.d_.stack.sz_); }

  void insert(T const& v) { push_back(v); }

  /**
     increase size without calling default ctor.
  */
  void append_unconstructed(size_type N) { resize_up_unconstructed(d_.stack.sz_ + N); }

  /**
     change size without calling default ctor.
  */
  void resize_unconstructed(size_type N) {
    if (d_.stack.sz_ > N)
      resize(N);
    else if (d_.stack.sz_ < N)
      resize_up_unconstructed(N);
  }

  /**
     insert hole of N elements at iterator i
  */
  void insert_hole(iterator where, size_type N) {
    if (where == end())
      append_unconstructed(N);
    else
      insert_hole_index(where - begin(), N);
  }

  /**
     insert hole of N elements at index i.
  */
  T* insert_hole_index(size_type i, size_type N) {
    size_type s = d_.stack.sz_;
    size_type snew = s + N;
    resize_up_unconstructed(snew);
    // TODO: optimize for snew, s inline/not
    T* v = begin();
    memmove_n(v + i + N, v + i, s - i);
    return v + i;
  }

  template <class ForwardIter>
  void insert(iterator where, ForwardIter i, ForwardIter e) {
    if (where == end())
      append(i, e);
    else
      insert_index((size_type)(where - begin()), i, e);
  }

  void insert(iterator where, T const* i, T const* e) {
    if (where == end())
      append(i, e);
    else
      insert_index((size_type)(where - begin()), i, e);
  }

  template <class ForwardIter>
  void insert_index(size_type atIndex, ForwardIter i, ForwardIter e) {
    size_type N = (size_type)std::distance(i, e);
    T* o = insert_hole_index(atIndex, N);
    for (; i != e; ++i, ++o) *o = *i;
  }

  void insert_index(size_type atIndex, T const* i, T const* e) {
    insert_index_range(atIndex, i, (size_type)(e - i));
  }

  void insert_index_range(size_type atIndex, T const* i, size_type N) {
    memcpy_n(insert_hole_index(atIndex, N), i, N);
  }

  void insert_index(size_type where, T const& t) { memcpy_n(insert_hole_index(where, 1), &t, 1); }

  void insert(iterator where, T const& t) { insert_index((size_type)(where - begin()), t); }

  /**
     O(n) of course.
  */
  void push_front(T const& t) { insert_index(0, t); }

  void insert_index(size_type where, size_type n, T const& t) {
    insert_hole_index(where, n);
    T* o = begin() + where;
    while (--n) *o++ = t;
  }

  void insert_index(iterator where, size_type n, T const& t) { insert_index(where - begin(), n, t); }

  void push_back_heap(T const& v) {
    if (d_.stack.sz_ == d_.heap.capacity_) ensure_capacity_grow(d_.stack.sz_ + 1);
    d_.heap.begin_[d_.stack.sz_++] = v;
  }

  void push_back(T const& v) {
    if (d_.stack.sz_ < kMaxInlineSize) {
      d_.stack.vals_[d_.stack.sz_] = v;
      ++d_.stack.sz_;
    } else if (d_.stack.sz_ == kMaxInlineSize) {
      copy_vals_to_ptr();
      d_.heap.begin_[kMaxInlineSize] = v;
      d_.stack.sz_ = kMaxInlineSize + 1;
    } else {
      push_back_heap(v);
    }
  }

  T& back() { return this->operator[](d_.stack.sz_ - 1); }
  T const& back() const { return this->operator[](d_.stack.sz_ - 1); }
  T& front() { return this->operator[](0); }
  T const& front() const { return this->operator[](0); }

  void pop_back() {
    assert(d_.stack.sz_ > 0);
    --d_.stack.sz_;
    if (d_.stack.sz_ == kMaxInlineSize) ptr_to_small();
  }

  /**
     take care of:
     free unused (heap) space.
  */
  void compact() {
    if (d_.stack.sz_ > kMaxInlineSize)  // was heap
      realloc_big(d_.stack.sz_);
  }

  /**
     like resize(newsz), but newsz must be <= size()
  */
  void compact(size_type newsz) {
    assert(newsz <= d_.stack.sz_);
    if (d_.stack.sz_ > kMaxInlineSize) {  // was heap
      d_.stack.sz_ = newsz;
      if (newsz <= kMaxInlineSize)  // now small
        ptr_to_small();
    } else  // was small already
      d_.stack.sz_ = newsz;
  }

  void resize(size_type s, T const& v = T()) {
    assert(s <= kMaxSize);
    assert(d_.stack.sz_ <= kMaxSize);
    if (s <= kMaxInlineSize) {
      if (d_.stack.sz_ > kMaxInlineSize) {
        d_.stack.sz_ = s;
        ptr_to_small();
        return;
      } else if (s <= d_.stack.sz_) {
      } else {  // growing but still small
        for (size_type i = d_.stack.sz_; i < s; ++i) d_.stack.vals_[i] = v;
      }
    } else {  // new s is heap
      if (s > d_.stack.sz_) {
        realloc_big(s);
        for (size_type i = d_.stack.sz_; i < s; ++i) d_.heap.begin_[i] = v;
      }
    }
    d_.stack.sz_ = s;
  }

  T& at_grow(size_type i) {
    size_type const oldsz = d_.stack.sz_;
    if (i >= oldsz) {
      size_type newsz = i + 1;
      resize_up_unconstructed(newsz);
      construct_range(begin(), oldsz, newsz);
    }
    return (*this)[i];
  }

  T& at_grow(size_type i, T const& zero) {
    size_type const oldsz = d_.stack.sz_;
    if (i >= oldsz) {
      size_type newsz = i + 1;
      resize_up_unconstructed(newsz);
      construct_range(begin(), oldsz, newsz, zero);
    }
    return (*this)[i];
  }

  friend inline T& atExpand(small_vector& v, size_type i) { return v.at_grow(i); }

  friend inline T& atExpand(small_vector& v, size_type i, T const& zero) { return v.at_grow(i, zero); }

  void set_grow(size_type i, T const& val) {
    size_type const oldsz = d_.stack.sz_;
    if (i >= oldsz) {
      size_type newsz = i + 1;
      resize_up_unconstructed(newsz);
      T* v = begin();
      construct_range(v, oldsz, i);
      new (&v[i]) T(val);
    } else
      (*this)[i] = val;
  }

  void set_grow(size_type i, T const& val, T const& zero) {
    size_type const oldsz = d_.stack.sz_;
    if (i >= oldsz) {
      size_type newsz = i + 1;
      resize_up_unconstructed(newsz);
      T* v = begin();
      construct_range(v, oldsz, i, zero);
      new (&v[i]) T(val);
    } else
      (*this)[i] = val;
  }

#if __GNUC__ > 8
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
  T& operator[](size_type i) {
    if (d_.stack.sz_ <= kMaxInlineSize) return d_.stack.vals_[i];
    return d_.heap.begin_[i];
  }
  T const& operator[](size_type i) const {
    if (d_.stack.sz_ <= kMaxInlineSize) return d_.stack.vals_[i];
    return d_.heap.begin_[i];
  }
#if __GNUC__ > 8
#pragma GCC diagnostic pop
#endif

  bool operator==(small_vector const& o) const {
    if (d_.stack.sz_ != o.d_.stack.sz_) return false;
    if (d_.stack.sz_ <= kMaxInlineSize) {
      for (size_type i = 0; i < d_.stack.sz_; ++i)
        if (d_.stack.vals_[i] != o.d_.stack.vals_[i]) return false;
      return true;
    } else {
      for (size_type i = 0; i < d_.stack.sz_; ++i)
        if (d_.heap.begin_[i] != o.d_.heap.begin_[i]) return false;
      return true;
    }
  }

  bool operator==(std::vector<T> const& other) const {
    if (d_.stack.sz_ != other.size()) return false;
    if (d_.stack.sz_ <= kMaxInlineSize) {
      for (size_type i = 0; i < d_.stack.sz_; ++i)
        if (d_.stack.vals_[i] != other[i]) return false;
      return true;
    } else {
      for (size_type i = 0; i < d_.stack.sz_; ++i)
        if (d_.heap.begin_[i] != other[i]) return false;
      return true;
    }
  }

  friend bool operator!=(small_vector const& a, small_vector const& b) { return !(a == b); }

  /**
     shortest-first total ordering (not lex.)
  */
  bool operator<(small_vector const& o) const { return compare_by_less<bool, true, false, false>(o); }

  /**
     shortest-first total ordering (not lex.)
  */
  bool operator<=(small_vector const& o) const { return compare_by_less<bool, true, true, false>(o); }

  /**
     shortest-first total ordering (not lex.)
  */
  bool operator>=(small_vector const& o) const { return compare_by_less<bool, false, true, true>(o); }

  /**
     shortest-first total ordering (not lex.)
  */
  bool operator>(small_vector const& o) const { return compare_by_less<bool, false, false, true>(o); }

  /**
     using T::operator<, return this <=> o :

     *this == o:  0.
     *this < o: -1
     *this > o: 1
     */
  int compare(small_vector const& o) const { return compare_by_less<int, -1, 0, 1>(o); }

  /**
     return iterator for matching element, if any, else end
  */
  template <class Target>
  T* find(Target const& target) const {
    return std::find(begin(), end(), target);
  }

  /** \return this->find(target)!=this->end() but might optimize better
   */
  template <class Target>
  bool contains(Target const& target) const {
    for (const_iterator i = begin(), e = end(); i != e; ++i)
      if (*i == target) return true;
    return false;
  }

  template <class Target>
  friend inline bool contains(small_vector const& vec, Target const& target) {
    return vec.contains(target);
  }

  typedef boost::iterator_range<iterator> iterator_range;
  typedef boost::iterator_range<const_iterator> const_iterator_range;

  iterator_range range() {
    return d_.stack.sz_ > kMaxInlineSize ? iterator_range(d_.heap.begin_, d_.heap.begin_ + d_.stack.sz_)
                                         : iterator_range(d_.stack.vals_, d_.stack.vals_ + d_.stack.sz_);
  }
  const_iterator_range range() const {
    return d_.stack.sz_ > kMaxInlineSize ? const_iterator_range(d_.heap.begin_, d_.heap.begin_ + d_.stack.sz_)
                                         : const_iterator_range(d_.stack.vals_, d_.stack.vals_ + d_.stack.sz_);
  }

  void swap(small_vector& o) { swap_pod(*this, o); }
  friend inline void swap(small_vector& a, small_vector& b) { return a.swap(b); }

  std::size_t hash_impl() const {
    using namespace boost;
    return (d_.stack.sz_ <= kMaxInlineSize) ? hash_range(d_.stack.vals_, d_.stack.vals_ + d_.stack.sz_)
                                            : hash_range(d_.heap.begin_, d_.heap.begin_ + d_.stack.sz_);
  }

  /**
     \return small_vector<..>(begin, end).hash_impl() without actually creating a
     copy of the substring
  */
  static inline std::size_t hash_substr(const_iterator begin, const_iterator end) {
    using namespace boost;
    return hash_range(begin, end);
  }

  // for boost::hash<>
  friend inline std::size_t hash_value(small_vector const& x) { return x.hash_impl(); }

  template <class Out>
  void print(Out& o) const {
    range_sep().print(o, begin(), end());
  }
  template <class Ch, class Tr>
  friend std::basic_ostream<Ch, Tr>& operator<<(std::basic_ostream<Ch, Tr>& o, small_vector const& self) {
    self.print(o);
    return o;
  }
  void init_unconstructed(size_type s) {
    assert(d_.stack.sz_ == 0);
    alloc(s);
  }

 private:
  void alloc(size_type s) {  // doesn't free old; for ctor. sets sz_
    d_.stack.sz_ = s;
    assert(s <= kMaxSize);
    if (s > kMaxInlineSize) {
      d_.heap.capacity_ = s;
      alloc_heap();
    }
  }
  static inline T* alloc_impl(size_type sz) { return (T*)std::malloc(sizeof(T) * sz); }
  static void free_impl(T* alloced) { std::free(alloced); }
  // pre: capacity_ is set
  void alloc_heap() {
    d_.heap.begin_ = alloc_impl(d_.heap.capacity_);  // TODO: boost user_allocator static template
  }
  void free_heap() const { free_impl(d_.heap.begin_); }
  void free() {
    if (d_.stack.sz_ > kMaxInlineSize) free_heap();
  }
  T* realloc(size_type s) {
    if (s == d_.stack.sz_) return begin();
    free();
    d_.stack.sz_ = s;
    if (s > kMaxInlineSize) {
      d_.heap.capacity_ = s;
      alloc_heap();
      return d_.heap.begin_;
    } else
      return d_.stack.vals_;
  }

  void realloc_big(size_type s) {
    assert(s > kMaxInlineSize);
    if (d_.stack.sz_ > kMaxInlineSize) {
      if (s > d_.heap.capacity_) ensure_capacity_grow(s);
    } else {
      T* tmp = alloc_impl(s);
      memcpy_n(tmp, d_.stack.vals_, d_.stack.sz_);
      d_.heap.capacity_ = s;  // note: these must be set AFTER copying from d_.stack.vals_
      d_.heap.begin_ = tmp;
    }
  }
  void ensure_capacity(size_type min_size) {
    // only call if you're already heap
    assert(min_size > kMaxInlineSize);
    assert(d_.stack.sz_ > kMaxInlineSize);
    if (min_size < d_.heap.capacity_) return;
    ensure_capacity_grow(min_size);
  }
  void ensure_capacity_grow(size_type min_size) {
#ifdef _MSC_VER
#undef max
#endif
    size_type new_cap = good_vector_size<T>::next(min_size);
    T* tmp = alloc_impl(new_cap);
    memcpy_n(tmp, d_.heap.begin_, d_.stack.sz_);
    free_heap();
    d_.heap.begin_ = tmp;  // note: these must be set AFTER copying from old vals
    d_.heap.capacity_ = new_cap;  // set after free_heap (though current allocator doesn't need old capacity)
  }

  void copy_vals_to_ptr() {
    assert(d_.stack.sz_ <= kMaxInlineSize);
#ifndef __clang_analyzer__
    T* const newHeapVals = alloc_impl(kInitHeapSize);  // note: must use tmp to not destroy d_.stack.vals_
    memcpy_n(newHeapVals, d_.stack.vals_,
#if GRAEHL_VALGRIND
             d_.stack.sz_
#else
             kMaxInlineSize  // may copy more than actual size_. ok. constant should be more optimizable
#endif
             );
    d_.heap.capacity_ = kInitHeapSize;
    d_.heap.begin_ = newHeapVals;
#endif
    // only call if you're going to immediately increase size_ to >kMaxInlineSize
  }
  void ptr_to_small() {
    assert(d_.stack.sz_ <= kMaxInlineSize);  // you decreased size_ already. normally ptr wouldn't be used
    // if size_ were this small
    T* fromHeap = d_.heap.begin_;  // should be no problem with memory access order (strict aliasing)
    // because of safe access through union
    for (size_type i = 0; i < d_.stack.sz_; ++i)  // no need to memcpy for small size
      d_.stack.vals_[i] = fromHeap[i];  // note: it was essential to save fromHeap first because
    // d_.stack.vals_ union-competes
    free_impl(fromHeap);
  }

  // o is longer. so if equal at end, then kLess.
  template <class Ret, int kLess, int kGreater>
  Ret compare_by_less_len_differs(small_vector const& o) {
    const_iterator e = end();
    std::pair<const_iterator, const_iterator> mo = std::mismatch(begin(), e, o.begin());
    if (mo.first == e) return kLess;
    return *mo.first < *mo.second ? kLess : kGreater;
  }
  // e.g. <: valLess=true, others = false. 3-value compare: Ret=int, kLess=-1, kGreater=1, kEqual=0
  //      (an arbitrary total ordering (not lex.))
  template <class Ret, int kLess, int kEqual, int kGreater>
  Ret compare_by_less(small_vector const& o) const {
    if (d_.stack.sz_ == o.d_.stack.sz_) {
      if (d_.stack.sz_ <= kMaxInlineSize) {
        for (size_type i = 0; i < d_.stack.sz_; ++i) {
          if (d_.stack.vals_[i] < o.d_.stack.vals_[i]) return kLess;
          if (o.d_.stack.vals_[i] < d_.stack.vals_[i]) return kGreater;
        }
        return kEqual;
      } else {
        for (size_type i = 0; i < d_.stack.sz_; ++i) {
          if (d_.heap.begin_[i] < o.d_.heap.begin_[i]) return kLess;
          if (o.d_.heap.begin_[i] < d_.heap.begin_[i]) return kGreater;
        }
        return kEqual;
      }
    }
    return std::lexicographical_compare(begin(), end(), o.begin(), o.end()) ? kLess : kGreater;
    // return (d_.stack.sz_ < o.d_.stack.sz_) ? kLess : kGreater; // faster but not lexicograph.
  }

  /* guarantee from c++ standard:

     one special guarantee is made in order to simplify the use of unions: If a POD-union contains several
     POD-structs that share a common initial sequence (9.2), and if an object of this POD-union type contains
     one of the POD-structs, it is permitted to inspect the common initial sequence of any of POD-struct
     members; see 9.2. ]
  */
  // anything that shrinks the array may mean copying back and forth, e.g. using small_vector as a stack with
  // repeated push/pop over and back to kMaxInlineSize
  /**
     this union forms the full contents of small_vector. it's probably best if kMaxInlineSize is at least 2
     (unless T is *extremely* large compared to size_type). you can experiment by checking
     sizeof(small_vector<...>) - the idea would be not to use the smaller of two kMaxInlineSize that both
     result in the same size union
  */
  // TODO: mv sz_ outside here? put dummy struct inside storage_union_variants for constexpr ctor?
  union storage_union_variants {
    size_type sz_only_;
    struct heap_storage_variant {
      size_type sz_;  // common prefix with small_storage.sz_
      size_type capacity_;  // only initialized when size_ > kMaxInlineSize. must not initialize when copying
      // from small.vals_ until after all are copied.
      T* begin_;  // otherwise
    } heap;
    struct inline_storage_variant {  // only initialized when size_ <= kMaxInlineSize
      size_type sz_;
      T vals_[kMaxInlineSize];  // iff size_<=kMaxInlineSize (note: tricky!
    } stack;
    // note: we have a shared initial sz_ member in each to ensure the best alignment/padding.
    // (otherwise it would be simpler to move sz_ outside the union
  };
  storage_union_variants d_;
};
}

namespace std {
template <class V, unsigned MaxInline, class Size>
inline void swap(graehl::small_vector<V, MaxInline, Size>& a, graehl::small_vector<V, MaxInline, Size>& b) {
  a.swap(b);
}

#ifdef _MSC_VER
#pragma warning(disable : 4099)
// msvc has class hash. should be struct. this hides warning.
#endif

template <class T>
struct hash;

template <class V, unsigned MaxInline, class Size>
struct hash<graehl::small_vector<V, MaxInline, Size>> {
 public:
  std::size_t operator()(graehl::small_vector<V, MaxInline, Size> const& vec) const {
    return vec.hash_impl();
  }
};
}

namespace boost {
namespace assign {

template <class V, unsigned MaxInline, class Size, class V2>
inline list_inserter<assign_detail::call_push_back<graehl::small_vector<V, MaxInline, Size>>, V>
operator+=(graehl::small_vector<V, MaxInline, Size>& c, V2 v) {
  return push_back(c)(v);
}
}

namespace serialization {

template <class V, unsigned MaxInline, class Size>
struct implementation_level<graehl::small_vector<V, MaxInline, Size>> {
  typedef mpl::int_<object_serializable> type;
  typedef mpl::integral_c_tag tag;
  BOOST_STATIC_CONSTANT(int, value = implementation_level::type::value);
};

/**
   since small_vector is a template,
   http://www.boost.org/doc/libs/1_48_0/libs/serialization/doc/traits.html#level
   recommends this to disable version tracking. note that object_serializable
   applies by default.
*/
template <class V, unsigned MaxInline, class Size>
struct tracking_level<graehl::small_vector<V, MaxInline, Size>> {
  typedef mpl::int_<track_never> type;
  typedef mpl::integral_c_tag tag;
  BOOST_STATIC_CONSTANT(int, value = tracking_level::type::value);
};
}
}  // ns

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#include <graehl/shared/warning_compiler.h>
CLANG_DIAG_IGNORE(tautological-undefined-compare)
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <sstream>

#define EXPECT_EQ_NOPRINT(x, y) BOOST_CHECK((x) == (y))

namespace graehl {
namespace unit_test {

std::size_t const test_archive_flags
    = boost::archive::no_header | boost::archive::no_codecvt | boost::archive::no_xml_tag_checking;

template <class InArchive, class Vec1, class Vec2>
void test_same_serialization_result(std::string const& str, Vec1 const& v, Vec2& v2) {
  std::stringstream ss(str);
  InArchive ia(ss, test_archive_flags);
  ia >> v2;
  BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(), v2.begin(), v2.end());
}

template <class Archive, class InArchive, class Vec1, class Vec2>
void test_same_serialization(Vec1 const& v, Vec2 const& v2) {
  Vec1 vb;
  Vec2 v2b;
  using namespace std;
  stringstream ss, ss2;
  Archive oa(ss, test_archive_flags), oa2(ss2, test_archive_flags);
  oa << v;
  oa2 << v2;
  string s = ss.str(), s2 = ss2.str();
  BOOST_REQUIRE_EQUAL(s, s2);
  test_same_serialization_result<InArchive>(s, v, vb);
  EXPECT_EQ_NOPRINT(vb, v);
  test_same_serialization_result<InArchive>(s, v, v2b);
  EXPECT_EQ_NOPRINT(v2b, v);
  test_same_serialization_result<InArchive>(s2, v2, vb);
  EXPECT_EQ_NOPRINT(v2, vb);
  test_same_serialization_result<InArchive>(s2, v2, v2b);
  EXPECT_EQ_NOPRINT(v2, v2b);
}

template <class SmallVecInt, class Archive, class InArchive>
void test_same_serialization(std::vector<int> const& v) {
  SmallVecInt sv(v.begin(), v.end());
  test_same_serialization<Archive, InArchive>(v, sv);
}

template <class SmallVecInt>
void test_same_serializations(std::vector<int> const& v) {
#if GRAEHL_SMALL_VECTOR_SERIALIZE_VERSION
  test_same_serialization<SmallVecInt, boost::archive::text_oarchive, boost::archive::text_iarchive>(v);
#else
  test_same_serialization<SmallVecInt, boost::archive::binary_oarchive, boost::archive::binary_iarchive>(v);
#endif
}

template <class SmallVecInt>
void test_small_vector_same_serializations() {
  std::vector<int> v;
  //  test_same_serializations(v);
  for (int i = 10; i < 19; ++i) {
    v.push_back(i);
    test_same_serializations<SmallVecInt>(v);
  }
}

template <class VectorInt>
void test_small_vector_1() {
  using namespace std;
  VectorInt v;
  VectorInt v2;
  v.push_back(0);
  v.push_back(1);
  EXPECT_EQ(v[1], 1);
  EXPECT_EQ(v[0], 0);
  v.push_back(2);
  EXPECT_EQ(v.size(), 3u);
  EXPECT_EQ(v[2], 2);
  EXPECT_EQ(v[1], 1);
  EXPECT_EQ(v[0], 0);
  v2 = v;
  VectorInt copy(v);
  EXPECT_EQ(copy.size(), 3u);
  EXPECT_EQ(copy[0], 0);
  EXPECT_EQ(copy[1], 1);
  EXPECT_EQ(copy[2], 2);
  EXPECT_EQ_NOPRINT(copy, v2);
  copy[1] = 99;
  EXPECT_TRUE(copy != v2);
  EXPECT_EQ(v2.size(), 3);
  EXPECT_EQ(v2[2], 2);
  EXPECT_EQ(v2[1], 1);
  EXPECT_EQ(v2[0], 0);
  v2[0] = -2;
  v2[1] = -1;
  v2[2] = 0;
  EXPECT_EQ(v2[2], 0);
  EXPECT_EQ(v2[1], -1);
  EXPECT_EQ(v2[0], -2);
  VectorInt v3(1, 1);
  EXPECT_EQ(v3[0], 1);
  v2 = v3;
  EXPECT_EQ(v2.size(), 1);
  EXPECT_EQ(v2[0], 1);
  VectorInt v4(10, 1);
  EXPECT_EQ(v4.size(), 10);
  EXPECT_EQ(v4[5], 1);
  EXPECT_EQ(v4[9], 1);
  v4 = v;
  EXPECT_EQ(v4.size(), 3);
  EXPECT_EQ(v4[2], 2);
  EXPECT_EQ(v4[1], 1);
  EXPECT_EQ(v4[0], 0);
  VectorInt v5(10, 2);
  EXPECT_EQ(v5.size(), 10);
  EXPECT_EQ(v5[7], 2);
  EXPECT_EQ(v5[0], 2);
  EXPECT_EQ(v.size(), 3);
  v = v5;
  EXPECT_EQ(v.size(), 10);
  EXPECT_EQ(v[2], 2);
  EXPECT_EQ(v[9], 2);
  VectorInt cc;
  for (int i = 0; i < 33; ++i) {
    cc.push_back(i);
    EXPECT_EQ(cc.size(), i + 1);
    EXPECT_EQ(cc[i], i);
    for (int j = 0; j < i; ++j) EXPECT_EQ(cc[j], j);
  }
  for (int i = 0; i < 33; ++i) EXPECT_EQ(cc[i], i);
  cc.resize(20);
  EXPECT_EQ(cc.size(), 20);
  for (int i = 0; i < 20; ++i) EXPECT_EQ(cc[i], i);
  cc[0] = -1;
  cc.resize(1, 999);
  EXPECT_EQ(cc.size(), 1);
  EXPECT_EQ(cc[0], -1);
  cc.resize(99, 99);
  for (int i = 1; i < 99; ++i) {
    // cerr << i << " " << cc[i] << '\n';
    EXPECT_EQ(cc[i], 99);
  }
  cc.clear();
  EXPECT_EQ(cc.size(), 0);
}

template <class VectorInt>
void test_small_vector_2() {
  using namespace std;
  VectorInt v;
  VectorInt v1(1, 0);
  VectorInt v2(2, 10);
  VectorInt v1a(2, 0);
  EXPECT_TRUE(v1 != v1a);
  EXPECT_EQ_NOPRINT(v1, v1);
  EXPECT_EQ(v1[0], 0);
  EXPECT_EQ(v2[1], 10);
  EXPECT_EQ(v2[0], 10);
  ++v2[1];
  --v2[0];
  EXPECT_EQ(v2[0], 9);
  EXPECT_EQ(v2[1], 11);

  // test shrinking/growing near max inline size (2)
  v2.emplace_back(12);
  EXPECT_EQ(v2[0], 9);
  EXPECT_EQ(v2[1], 11);
  EXPECT_EQ(v2.size(), 3u);
  EXPECT_EQ(v2[2], 12);
  v2.pop_back();
  EXPECT_EQ(v2.size(), 2u);
  EXPECT_EQ(v2[0], 9);
  EXPECT_EQ(v2[1], 11);
  v2.pop_back();
  EXPECT_EQ(v2.size(), 1u);
  EXPECT_EQ(v2[0], 9);
  v2.emplace_back(11);
  EXPECT_EQ(v2.size(), 2u);
  EXPECT_EQ(v2[0], 9);
  EXPECT_EQ(v2[1], 11);

  VectorInt v3(v2);
  EXPECT_EQ(v3[0], 9);
  EXPECT_EQ(v3[1], 11);
  assert(!v3.empty());
  EXPECT_EQ(v3.size(), 2);
  v3.clear();
  assert(v3.empty());
  EXPECT_EQ(v3.size(), 0);
  EXPECT_TRUE(v3 != v2);
  EXPECT_TRUE(v2 != v3);
  v3 = v2;
  EXPECT_EQ_NOPRINT(v3, v2);
  EXPECT_EQ_NOPRINT(v2, v3);
  EXPECT_EQ(v3[0], 9);
  EXPECT_EQ(v3[1], 11);
  assert(!v3.empty());
  EXPECT_EQ(v3.size(), 2);
  VectorInt v4 = v3;
  VectorInt v3b = v3;
  v3.append(v4.begin() + 1, v4.end());
  EXPECT_EQ(v3.size(), 3);
  EXPECT_EQ(v3[2], 11);
  v3b.insert(v3b.end(), v4.begin() + 1, v4.end());
  EXPECT_EQ(v3b.size(), 3);
  EXPECT_EQ(v3b[2], 11);

  v4 = v3;  // avoid aliasing
  v3.append(v4.begin(), v4.end());
  v4 = v3;
  EXPECT_EQ(v3.size(), 6);
  v3.append((int const*)v4.begin(), (int const*)v4.end());
  EXPECT_EQ(v3.size(), 12);
  EXPECT_EQ(v3[11], 11);
  VectorInt v5(10);
  EXPECT_EQ(v5[1], 0);
  EXPECT_EQ(v5.size(), 10);
  v3.assign(v4.begin(), v4.end());
  EXPECT_EQ_NOPRINT(v3, v4);
  int i = 2;

  v4.assign(&i, &i);
  EXPECT_EQ(v4.size(), 0);
  v4.assign(v1.begin(), v1.end());
  EXPECT_EQ_NOPRINT(v4, v1);
  v4.assign(&i, &i + 1);
  EXPECT_EQ(v4.size(), 1);
  EXPECT_EQ(v4[0], i);

  v4.insert(v4.begin(), 1);
  EXPECT_EQ(v4.size(), 2);
  EXPECT_EQ(v4[0], 1);
  EXPECT_EQ(v4[1], i);

  VectorInt v4c = v4;
  v4.insert(v4.begin(), v4c.begin(), v4c.end());
  EXPECT_EQ(v4.size(), 4);
  EXPECT_EQ(v4[0], 1);
  EXPECT_EQ(v4[2], 1);
  EXPECT_EQ(v4[1], i);
  EXPECT_EQ(v4[3], i);

  v4.push_front(4);
  EXPECT_EQ(v4.size(), 5);
  EXPECT_EQ(v4[0], 4);
  EXPECT_EQ(v4[3], 1);
  EXPECT_EQ(v4[2], i);
  EXPECT_EQ(v4[4], i);

  VectorInt vg = v4;
  EXPECT_TRUE(vg <= v4);
  EXPECT_TRUE(vg >= v4);
  EXPECT_TRUE(!(vg < v4));
  EXPECT_TRUE(!(v4 < vg));

  vg[2] = i + 1;
  EXPECT_TRUE(vg >= v4);
  EXPECT_TRUE(vg > v4);
  EXPECT_TRUE(v4 < vg);
  vg[2] = v4[2] - 1;
  vg[1] = v4[1] + 1;
  EXPECT_TRUE(vg >= v4);
  EXPECT_TRUE(vg > v4);
  EXPECT_TRUE(v4 < vg);
  vg[0] = v4[1] - 1;
  EXPECT_TRUE(v4 >= vg);
  EXPECT_TRUE(v4 > vg);
  EXPECT_TRUE(vg < v4);
}

// cppcheck-suppress syntaxError
BOOST_AUTO_TEST_CASE(test_small_vector_larger_than_2) {
  test_small_vector_1<small_vector<int, 1>>();
  test_small_vector_1<small_vector<int, 3, std::size_t>>();
  test_small_vector_1<small_vector<int, 5, unsigned short>>();
}

BOOST_AUTO_TEST_CASE(test_small_vector_small) {
  test_small_vector_2<small_vector<int, 1>>();
  test_small_vector_2<small_vector<int, 3, std::size_t>>();
  test_small_vector_2<small_vector<int, 5, unsigned short>>();
}

BOOST_AUTO_TEST_CASE(small_vector_compatible_serialization) {
  test_small_vector_same_serializations<small_vector<int, 1>>();
  test_small_vector_same_serializations<small_vector<int, 3, std::size_t>>();
  test_small_vector_same_serializations<small_vector<int, 5, unsigned short>>();
}


}}

#endif
// GRAEHL_TEST


#endif
