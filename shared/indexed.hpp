/** \file

    like a map<T, size_t> and a vector<T> such that vector[map[x]] == x for all
    x that were added. (used by BasicVocabularyImpl with T = string, for example)
*/

#ifndef GRAEHL_SHARED__INDEXED_JG_2014_03_04_HPP
#define GRAEHL_SHARED__INDEXED_JG_2014_03_04_HPP

#include <graehl/shared/stable_vector.hpp>
#include <boost/functional/hash.hpp>
#include <algorithm>
#include <cassert>
#include <boost/cstdint.hpp>

#ifndef INDEXED_EXTRA_ASSERT
# define INDEXED_EXTRA_ASSERT 0
#endif

namespace graehl {

double const kMaxIndexedHashLoad = 0.7; // will double capacity when load reaches this
double const kOneOverMaxIndexedHashLoad = 1. / kMaxIndexedHashLoad;

inline void mixbits(boost::uint64_t &h) {
  h ^= h >> 23;
  h *= 0x2127599bf4325c37ULL;
  h ^= h >> 47;
}

template <class Int>
inline bool is_power_of_2(Int i) {
  return (i & (i-1)) == 0;
}

/// return power of 2 >= x
inline boost::uint32_t next_power_of_2(boost::uint32_t x) {
  assert(x <= (1 << 30));
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  ++x;
  assert(is_power_of_2(x));
  return x;
}

/// return power of 2 >= x
inline boost::uint64_t next_power_of_2(boost::uint64_t x) {
  assert(x <= (1ULL << 60));
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x |= x >> 32;
  ++x;
  assert(is_power_of_2(x));
  return x;
}

template <class I>
I hash_capacity_for_size(I sz) {
  I atleast = (I)(sz * kOneOverMaxIndexedHashLoad);
  return next_power_of_2(atleast + 1);
}

template <class T>
struct indexed_traits {
  template <class T2>
  inline bool operator()(T const& a, T2 const& b) const { return a == b; }
  inline std::size_t operator()(T const& a) const { return boost::hash<T>()(a); }
};

template <class T,
          class IndexT = unsigned,
          class Vector = stable_vector<T>,
          class HashEqualsTraits = indexed_traits<T>
          >
struct indexed : HashEqualsTraits {
  typedef IndexT I;
  enum { kStartHashCapacity = 1 << 16 };
  I growAt;

  indexed(I startHashCapacity = (I)kStartHashCapacity)
      : mask_()
      , index_()
  {
    init_empty_hash(next_power_of_2(startHashCapacity));
  }

  void reserve(I capacity) {
    vals_.reserve(capacity);
  }

  typedef I *Indices;

  enum { kNullIndex = -1 };
  typedef Vector Vals;

  /// overwriting old values w/ different ones will lead to an overcrowded hash
  /// table that takes longer to search (until you call rehash)
  template <class T2>
  void set(I i, T2 const& val, bool mustBeNew = true) {
    grow(vals_, i) = val;
    add_index(i, vals_[i]);
  }

  /// these added items won't be locatable until you call rehash
  template <class T2>
  void setDeferHash(I i, T2 const& val) {
    grow(vals_, i) = val;
  }

  template <class T2>
  I find(T2 const& val, I otherwise = (I)kNullIndex) const {
    check();
    //TODO: use pointers into index array instead of indices into it - might be slightly faster if we cache end pointer as member
    for (I i = find_start(val); ;) {
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
  template <class T2>
  I index(T2 const& val) {
    check();
    I sz = vals_.size();
    if (sz >= growAt)
      rehash_pow2(hash_capacity_for_size(sz + 1));
    assert(sz < growAt);
    for (I i = find_start(val); ;) {
      I &j = index_[i];
      if (j == (I)kNullIndex) {
        vals_.push_back(val);
        return (j = sz);
      } else if (HashEqualsTraits::operator()(vals_[j], val))
        return j;
      else if (i == mask_)
        i = 0;
      else
        ++i;
    }
  }

  T const& operator[](I i) const {
    return vals_[i];
  }

  void rehash() {
    rehash(size());
  }

  void rehash(I forSize) {
    rehash_pow2(hash_capacity_for_size(forSize));
  }

  void rehash_pow2(I capacityPowerOf2) {
    init_empty_hash(capacityPowerOf2);
    setGrowAt(capacityPowerOf2);
    for (I vali = 0, valn = vals_.size(); vali < valn; ++vali)
      add_index(vali, vals_[vali]);
    check();
  }

  /// for setDeferHash where you've left some gaps (default constructed elements in vals_)
  void rehash_except(T const& except = T()) {
    rehash_except(size(), except);
  }

  void rehash_except(I forSize, T const& except = T()) {
    rehash_except_pow2(hash_capacity_for_size(forSize), except);
  }

  /// for setDeferHash where you've left some gaps (default constructed elements in vals_)
  void rehash_except_pow2(I capacityPowerOf2, T const& except = T()) {
    init_empty_hash(capacityPowerOf2);
    for (I vali = 0, valn = vals_.size(); vali < valn; ++vali) {
      T const& val = vals_[vali];
      if (val != except)
        add_index(vali, val);
    }
    check();
  }

  ~indexed() {
    free_hash();
  }

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

  I size() const {
    return vals_.size();
  }

  void check() const {
    assert(growAt < mask_);
    assert(vals_.size() <= growAt);
#if INDEXED_EXTRA_ASSERT
    assert(valid());
#else
#endif
  }

  /// \return true if everything was added with index() since last rehash and no duplicates were ever added via set()
  bool valid() const {
    return count_indexed() == size();
  }
 private:

  void add_index(I add, T const& val) {
    assert(vals_[add] == val);
    add_index_start(add, find_start(val));
  }

  /// pre: index must not be full, and 'add' must not be in hash yet (will be
  /// caught by debug assert only).
  void add_index_start(I add, I start) {
    assert(growAt < mask_);
    for (I i = start; ;) {
      I &j = index_[i];
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

  template <class T2>
  I find_start(T2 const& val) const {
    boost::uint64_t h = HashEqualsTraits::operator()(val);
    mixbits(h);
    return h & mask_;
  }

  I count_indexed() const {
    I n = 0;
    for (I i = 0 ; i <= mask_; ++i)
      if (index_[i] != (I)kNullIndex)
        ++n;
    return n;
  }

  /// this never shrinks our previously allocated capacity
  void init_empty_hash(I capacityPowerOf2) {
    if (capacityPowerOf2 < 2)
      capacityPowerOf2 = 2;
    assert(is_power_of_2(capacityPowerOf2));
    I const newmask = capacityPowerOf2 - 1;
    if (!index_ || mask_ < newmask) {
      free_hash();
      mask_ = newmask;
      index_ = (Indices)::operator new(sizeof(I) * capacityPowerOf2);
      setGrowAt(capacityPowerOf2);
    }
    clear_hash();
  }

  void setGrowAt(I capacityPowerOf2) {
    growAt = (I)(kMaxIndexedHashLoad * capacityPowerOf2);
    if (growAt >= mask_) growAt = mask_ - 1;
  }

  void clear_hash() {
    if (index_)
      std::fill(index_, index_ + mask_ + 1, (I)kNullIndex);
  }

  void free_hash() {
    ::operator delete((void*)index_);
  }

  I mask_; // capacity - 1
  Vals vals_;
  Indices index_;
};


}

#endif // GRAEHL_SHARED__INDEXED_JG_2014_03_04_HPP
