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

    like a map<T, size_t> and a vector<T> such that vector[map[x]] == x for all
    x that were added. (used by BasicVocabularyImpl with T = string, for example)
*/

#ifndef GRAEHL_SHARED__INDEXED_JG_2014_03_04_HPP
#define GRAEHL_SHARED__INDEXED_JG_2014_03_04_HPP
#pragma once

#include <graehl/shared/stable_vector.hpp>
#include <boost/functional/hash.hpp>
#include <algorithm>
#include <cassert>
#include <string>
#include <graehl/shared/int_types.hpp>
#include <graehl/shared/farmhash.hpp>

#ifndef GRAEHL_INDEXED_EXTRA_ASSERT
#define GRAEHL_INDEXED_EXTRA_ASSERT 0
#endif

namespace graehl {

double const kMaxIndexedHashLoad = 0.666;  // will double capacity when load reaches this
double const kOneOverMaxIndexedHashLoad = 1. / kMaxIndexedHashLoad;

template <class I>
I hash_capacity_for_size(I sz) {
  I atleast = (I)(sz * kOneOverMaxIndexedHashLoad);
  return next_power_of_2(atleast + 1);
}

template <class T>
struct indexed_traits {
#if GRAEHL_CPP11
  template <class Cont, class Key>
  void push_back(Cont& c, Key&& val) {
    c.emplace_back(std::forward<Key>(val));
  }
#else
  template <class Cont, class Key>
  void push_back(Cont& c, Key const& val) {
    c.push_back(val);
  }
#endif
  template <class Key>
  bool operator()(T const& a, Key const& b) const {
    return a == b;
  }
  template <class Key>
  std::size_t operator()(Key const& a) const {
    return boost::hash<T>()(a);
  }
};

template <>
struct indexed_traits<std::string> {
  typedef std::string T;
#if GRAEHL_CPP11
  template <class Cont>
  void push_back(Cont& c, T&& val) {
    c.emplace_back(std::move(val));
  }
  template <class Cont>
  void push_back(Cont& c, T const& val) {
    c.emplace_back(val);
  }
  template <class Cont>
  void push_back(Cont& c, char const* val) {
    c.emplace_back(val);
  }
#endif
  template <class Cont, class Key>
  void push_back(Cont& c, Key const& val) {
#if GRAEHL_CPP11
    c.emplace_back(val.data(), val.length());
#else
    c.push_back(val);
#endif
  }

  template <class Key>
  bool operator()(T const& a, Key const& b) const {
    std::size_t sz = a.size();
    return sz == b.size() && !std::memcmp(a.data(), b.data(), sz);
  }
  bool operator()(T const& a, char const* s) const { return !std::strcmp(a.c_str(), s); }

  template <class Key>
  std::size_t operator()(Key const& s) const {
    return farmhash(s.data(), s.size());
  }
  std::size_t operator()(char const* s) const { return farmhash(s, std::strlen(s)); }
};

template <class T, class IndexT = unsigned, class Vector = stable_vector<T>,
          class HashEqualsTraits = indexed_traits<T> >
struct indexed : HashEqualsTraits {
  typedef IndexT I;
  typedef I* Indices;
  enum { kNullIndex = -1 };
  typedef T Val;
  typedef Vector Vals;
  enum { kStartHashCapacity = 1 << 16 };

  indexed(indexed const& o) : mask_(o.mask_), vals_(o.vals_), index_(), growAt_(o.growAt_) {
    if (o.index_) {
      std::size_t bytes = (mask_ + 1) * sizeof(I);
      std::memcpy((index_ = (Indices)std::malloc(bytes)), o.index_, bytes);
    }
  }

  indexed(I startHashCapacity = (I)kStartHashCapacity) : mask_(), index_() {
    init_empty_hash(next_power_of_2(startHashCapacity));
  }

  /// if !index, can't use find/index methods (like after you call free_hash()) until rehash()
  explicit indexed(Vals const& vals, bool index = true) : mask_(), index_(), vals_(vals) {
    if (index) rehash();
  }

  void reserve(I capacity) { vals_.reserve(capacity); }

  /// you can no longer use find/index (only operator[](index)) until rehash() - in fact, you
  /// can just use the returned Vals vector directly
  Vals& freehash() {
    ::std::free((void*)index_);
    index_ = NULL;
    return vals_;
  }

  Vals& vals() { return vals_; }

  /// overwriting old values w/ different ones will lead to an overcrowded hash
  /// table that takes longer to search (until you call rehash)
  template <class Key>
  void set(I i, Key const& val, bool mustBeNew = true) {
    grow(vals_, i) = val;
    add_index(i, vals_[i]);
  }

  /// these added items won't be locatable until you call rehash
  template <class Key>
  void setDeferHash(I i, Key const& val) {
    grow(vals_, i) = val;
  }

  // TODO: template find,index for compatible hash of e.g. pair<char*, char*>
  // vs. string without first creating string - different method name since wrong
  // hash fn selection would surprise user

  template <class Key>
  I find(Key const& val, I otherwise = (I)kNullIndex) const {
    check_index();
    // TODO: use pointers into index array instead of indices into it - might be slightly faster if we cache
    // end pointer as member
    for (I i = find_start(val);;) {
      I const j = index_[i];
      if (j == (I)kNullIndex)
        return otherwise;
      else if (HashEqualsTraits::operator()(vals_[j], val))
        return j;
      else if (i == mask_)
        i = 0;
      else
        ++i;
    }
  }

  /// like find but adds if wasn't already present
  template <class Key>
  I index(Key const& val) {
    check_index();
    I sz = vals_.size();
    if (sz >= growAt_) rehash_pow2(hash_capacity_for_size(sz + 1));
    assert(sz < growAt_);
    for (I i = find_start(val);;) {
      I& j = index_[i];
      if (j == (I)kNullIndex) {
        HashEqualsTraits::push_back(vals_, val);
        return (j = sz);
      } else if (HashEqualsTraits::operator()(vals_[j], val))
        return j;
      else if (i == mask_)
        i = 0;
      else
        ++i;
    }
  }

  T const& operator[](I i) const { return vals_[i]; }

  T const& at(I i) const { return vals_.at(i); }

  bool hashed() const { return index_; }

  void rehash() { rehash(size()); }

  void rehash(I forSize) { rehash_pow2(hash_capacity_for_size(forSize)); }

  void rehash_pow2(I capacityPowerOf2) {
    init_empty_hash(capacityPowerOf2);
    setGrowAt(capacityPowerOf2);
    for (I vali = 0, valn = vals_.size(); vali < valn; ++vali) add_index(vali, vals_[vali]);
    check_index();
  }

  /// for setDeferHash where you've left some gaps (default constructed elements in vals_)
  void rehash_except(T const& except = T()) { rehash_except(size(), except); }

  void rehash_except(I forSize, T const& except = T()) {
    rehash_except_pow2(hash_capacity_for_size(forSize), except);
  }

  /// for setDeferHash where you've left some gaps (default constructed elements in vals_)
  void rehash_except_pow2(I capacityPowerOf2, T const& except = T()) {
    init_empty_hash(capacityPowerOf2);
    for (I vali = 0, valn = vals_.size(); vali < valn; ++vali) {
      T const& val = vals_[vali];
      if (val != except) add_index(vali, val);
    }
    check_index();
  }

  ~indexed() { freehash(); }

  void shrink(I newsize) {
    assert(newsize <= size());
    if (newsize <= size()) {
      shrink_destroy(vals_, newsize);
      rehash();
      assert(newsize == size());
    }
  }

  void clear() {
    clear_destroy(vals_);
    clear_hash();
  }

  I size() const { return vals_.size(); }

  template <class Key>
  bool contains(Key const& val) const {
    check_index();
    // TODO: use pointers into index array instead of indices into it - might be slightly faster if we cache
    // end pointer as member
    for (I i = find_start(val);;) {
      I const j = index_[i];
      if (j == (I)kNullIndex)
        return false;
      else if (HashEqualsTraits::operator()(vals_[j], val))
        return true;
      else if (i == mask_)
        i = 0;
      else
        ++i;
    }
  }

  bool known(I i) const { return i < vals_.size(); }

  void check_index() const {
    assert(index_);
    assert(growAt_ < mask_);
    assert(vals_.size() <= growAt_);
#if GRAEHL_INDEXED_EXTRA_ASSERT
    assert(valid_index());
#endif
  }

  /// \return true if everything was added with index() since last rehash and no duplicates were ever added
  /// via set()
  bool valid_index() const { return count_indexed() == size(); }

  bool empty() const { return vals_.empty(); }

  template <class VisitNameId>
  void visit(VisitNameId const& v) {
    for (I i = 0, e = size(); i < e; ++i) v(vals_[i], i);
  }

 private:
  void add_index(I add, T const& val) {
    assert(vals_[add] == val);
    add_index_start(add, find_start(val));
  }

  /// pre: index must not be full, and 'add' must not be in hash yet (will be
  /// caught by debug assert only).
  void add_index_start(I add, I start) {
    assert(growAt_ < mask_);
    for (I i = start;;) {
      I& j = index_[i];
      if (j == (I)kNullIndex) {
        j = add;
        return;
      }
      assert(j != add);
      if (i == mask_)
        i = 0;
      else
        ++i;
    }
  }

  template <class Key>
  I find_start(Key const& val) const {
    return HashEqualsTraits::operator()(val) & mask_;
  }

  I count_indexed() const {
    I n = 0;
    for (I i = 0; i <= mask_; ++i)
      if (index_[i] != (I)kNullIndex) ++n;
    return n;
  }

  /// this never shrinks our previously allocated capacity
  void init_empty_hash(I capacityPowerOf2) {
    if (capacityPowerOf2 < 4) capacityPowerOf2 = 4;
    assert(is_power_of_2(capacityPowerOf2));
    I const newmask = capacityPowerOf2-1;
    if (!index_ || mask_ < newmask) {
      freehash();
      mask_ = newmask;
      index_ = (Indices)std::malloc(sizeof(I) * capacityPowerOf2);
      setGrowAt(capacityPowerOf2);
    }
    clear_hash();
  }

  void setGrowAt(I capacityPowerOf2) {
    growAt_ = (I)(kMaxIndexedHashLoad * capacityPowerOf2);
    if (growAt_ >= mask_) growAt_ = mask_-1;
  }

  void clear_hash() {
    if (index_) std::fill(index_, index_ + mask_ + 1, (I)kNullIndex);
  }

  I mask_;  // capacity - 1
  Vals vals_;
  Indices index_;
  I growAt_;
};


}

#endif
