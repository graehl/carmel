/** \file

    a large vector without contiguous storage that never copies after initial
    insert. implemented as a vector of heap-allocated chunks - similar to
    ptr_vector except objects are held by value and there's less overhead in
    pointers and small allocations of individual values

    iterators are also stable.

    i've only added the vector-interface parts i need right now (random access w/
    growing and push_back), but the full set of them would be efficient/easy
*/

#ifndef GRAEHL_SHARED__STABLE_VECTOR
#define GRAEHL_SHARED__STABLE_VECTOR

#include <vector>
#include <memory>
#include <graehl/shared/small_vector.hpp>

namespace graehl {

template <class T>
inline void reinit(T &val) {
  val.~T();
  new (&val) T;
}

template <class T, class IndexT = unsigned, unsigned Log2ChunkSize = 6, bool UseSmallVector = false>
struct stable_vector {
  enum { kRemoveDestroys = true };
  typedef IndexT I;
  typedef T value_type;
  typedef stable_vector self_type;

  enum { chunkshift = Log2ChunkSize,
         chunksize = (1 << Log2ChunkSize),
         posmask = (chunksize - 1) };

  typedef T *Chunk;

  typedef typename use_small_vector<Chunk, UseSmallVector>::type Chunks;
  Chunks chunks;
  I size_, capacity; // capacity is redundant but helps w/ speed

  T &operator[](I i) {
    assert(i < size_);
    return chunks[i >> chunkshift][i & posmask];
  }
  T const &operator[](I i) const {
    assert(i < size_);
    return chunks[i >> chunkshift][i & posmask];
  }

  T &operator()(I chunk, I pos) {
    assert(pos < chunksize);
    assert(chunk < chunks.size());
    return chunks[chunk][pos];
  }
  T const& operator()(I chunk, I pos) const {
    assert(pos < chunksize);
    assert(chunk < chunks.size());
    return chunks[chunk][pos];
  }

  stable_vector() : size_(), capacity() {}
  stable_vector(I size) {
    init(size);
  }
  stable_vector(I size, T const& initial) {
    init(size, initial);
  }

  /// post: size() is exactly size, and any new elements needed (up to capacity,
  /// not size) are constructed as 'initial'
  void resize(I size, T const& initial) {
    if (size < size_) {
      truncate(size);
      return;
    }
    if (!size_) {
      init(size, initial);
      return;
    }
    I nowchunk = size_ >> chunkshift;
    I nowpos = size_ & posmask;
    I afterchunk = size >> chunkshift;
    I afterpos = size & posmask;
    Chunk last = chunks[nowchunk];
    I morechunks = afterchunk - nowchunk;
    // fill up this chunk
    for (I i = nowpos, n = morechunks ? chunksize : afterpos; i < n; ++i)
      last[i] = initial;

    if (morechunks) {
      for (I i = 0, n = morechunks; i < n; ++i)
        push_back_chunk(new_chunk(initial)); // additional full chunks
    }
    size_ = size;
  }

  void reserve(I cap) {
    if (cap > capacity)
      reserve_chunks((cap + posmask) >> chunkshift);
    assert(capacity >= cap);
  }
  void reserve_chunks(I nchunks) {
    I oldchunks = (I)chunks.size();
    if (nchunks < oldchunks) return;
    chunks.reserve(nchunks);
    for (I n = nchunks - oldchunks; n; --n)
      push_back_chunk();
  }

  /// position within chunk
  I chunk(I i) {
    return i >> chunkshift;
  }
  inline I pos(I i) {
    return i & posmask;
  }

  template <class Val>
  struct iterator_impl {
    typedef Val value_type;
    typedef Val & reference;
    self_type &self;
    I i;
    iterator_impl &operator++() {
      ++i;
      return *this;
    }
    void operator++(int) {
      ++i;
    }
    iterator_impl &operator--() {
      --i;
      return *this;
    }
    void operator--(int) {
      --i;
    }
    void operator+=(I delta) {
      i += delta;
    }
    void operator-=(I delta) {
      i -= delta;
    }
    Val &operator *() const {
      return self[i]; //TODO: we can optimize using two separate indices
    }

#define GRAEHL_STABLE_VECTOR_RELATION(rel)              \
    bool operator rel (iterator_impl const& o) const {  \
      assert(&self == &o.self);                         \
      return i rel o.i;                                 \
    }

    GRAEHL_STABLE_VECTOR_RELATION(==)
    GRAEHL_STABLE_VECTOR_RELATION(!=)
    GRAEHL_STABLE_VECTOR_RELATION(<=)
    GRAEHL_STABLE_VECTOR_RELATION(>=)
    GRAEHL_STABLE_VECTOR_RELATION(<)
    GRAEHL_STABLE_VECTOR_RELATION(>)
#undef GRAEHL_STABLE_VECTOR_RELATION

    iterator_impl(self_type const& self, I i)
    : self(const_cast<self_type &>(self)), i(i)
    {}
  };
  typedef iterator_impl<T> iterator;
  typedef iterator_impl<T const> const_iterator;
  iterator begin() {
    return iterator(*this, 0);
  }
  const_iterator begin() const {
    return const_iterator(*this, 0);
  }
  iterator end() {
    return iterator(*this, size_);
  }
  const_iterator end() const {
    return const_iterator(*this, size_);
  }

  bool empty() const {
    return !size_;
  }
  I size() const {
    return size_;
  }

  void push_back() {
    if (size_ == capacity)
      push_back_chunk();
    ++size_;
  }

  template <class Tcopy>
  void push_back(Tcopy const& val) {
    if (size_ == capacity) {
      push_back_chunk();
      assert((size_ & posmask) == 0);
      chunks.back()[0] = val;
    } else
      chunks.back()[size_ & posmask] = val;
    ++size_;
  }

  void pop_back(bool destroy = kRemoveDestroys) {
    if (destroy) {
      reinit((*this)[--size_]);
    } else
      --size_;
  }

  void clear(bool destroy = kRemoveDestroys) {
    if (destroy) {
      for (typename Chunks::const_iterator i = chunks.begin(), e = chunks.end(); i!=e; ++i)
        free_chunk(*i);
      chunks.clear();
    }
    size_ = 0;
  }

  /// post: size() is no more than sz
  void truncate(I sz, bool destroy = kRemoveDestroys) {
    if (sz < size_) {
      I const oldchunks = (I)chunks.size();
      I newchunks = ((sz + posmask) >> chunkshift);
      for (I i = newchunks; i < oldchunks; ++i)
        free_chunk(chunks[i]);
      chunks.resize(newchunks);
      capacity = newchunks * chunksize;
      if (destroy && newchunks) {
        assert(!chunks.empty());
        Chunk &chunk = chunks.back();
        for (I pos = sz & posmask; pos < chunksize; ++pos)
          reinit(chunk[pos]);
      }
      size_ = sz;
    }
  }

  /// post: size() is exactly sz
  void resize(I sz) {
    if (sz > size_) {
      reserve(sz);
      size_ = sz;
    } else
      truncate(sz, kRemoveDestroys);
  }

  friend inline void clear_destroy(stable_vector &v) {
    v.clear(true);
  }

  friend inline void shrink_destroy(stable_vector &v, I sz) {
    v.truncate(sz, true);
  }

  /**
     grows size so that i is < size.
  */
  friend inline
  T &grow(stable_vector &v, I i) {
    return v.growImpl(i);
  }

  ~stable_vector() {
    for (typename Chunks::const_iterator i = chunks.begin(), e = chunks.end(); i!=e; ++i)
      free_chunk(*i);
  }

  stable_vector(stable_vector const& o) : size_(o.size_), capacity() {
    for (typename Chunks::const_iterator i = o.chunks.begin(), e = o.chunks.end(); i!=e; ++i)
      push_back_chunk(clone_chunk(*i));
  }

  void operator = (stable_vector const& o) {
    if (this == &o) return;
    clear();
    for (typename Chunks::const_iterator i = o.chunks.begin(), e = o.chunks.end(); i!=e; ++i)
      push_back_chunk(clone_chunk(*i));
    size_ = o.size_;
  }

  bool operator != (stable_vector const& o) const {
    return !(*this == o);
  }

  bool operator == (stable_vector const& o) const {
    if (size_ != o.size_)
      return false;
    I nchunks = chunks.size();
    if (o.chunks.size() != nchunks)
      return false;
    I i = 0, n = nchunks - 1;
    for (; i < n; ++i)
      if (!equal_chunks(chunks[i], o.chunks[i]))
        return false;
    return equal_chunks(chunks[i], o.chunks[i], size_ & posmask); // last chunk
  }

 private:
  void init(I size) {
    capacity = 0;
    reserve(size);
  }

  void init(I size, T const& initial) {
    capacity = 0;
    for (I i = 0, n = size >> chunkshift; i < n; ++i)
      push_back_chunk(initial);
    push_back_chunk(initial, size & posmask);
    size_ = size;
  }

  T &growImpl(I i) {
    if (i >= size_)
      reserve((size_ = i + 1));
    return (*this)[i];
  }

  /**
     for simplicitly, we immediately default construct every allocated chunk as
     a valid object, and require assignment operator instead of copy
     constructor. it would be easy to change to lazy-copy-construct later,
     though
  */
  void push_back_chunk() {
    push_back_chunk(new_chunk());
  }
  void push_back_chunk(T const& initial, I sz = chunksize) {
    push_back_chunk(new_chunk(initial, sz));
  }

  bool equal_chunks(Chunk a, Chunk b, I sz = chunksize) const {
    for (I i = 0; i < sz; ++i)
      if (a[i] != b[i])
        return false;
    return true;
  }

  static Chunk new_chunk() {
    Chunk c = (Chunk)::operator new(sizeof(T)*chunksize);
    I i = 0;
    try {
      for (; i < chunksize; ++i)
        new (&c[i]) T;
      return c;
    } catch (...) {
      while (i)
        c[--i].~T();
      ::operator delete((void*)c);
      throw std::runtime_error("stable_vector constructing chunk");
      return 0;
    }
  }
  static Chunk new_chunk(T const& initial, I sz = chunksize) {
    Chunk c = (Chunk)::operator new(sizeof(T)*chunksize);
    I i = 0;
    try {
      for (; i < sz; ++i)
        new (&c[i]) T(initial);
      for (; i < chunksize; ++i)
        new (&c[i]) T;
      return c;
    } catch (...) {
      while (i)
        c[--i].~T();
      ::operator delete((void*)c);
      throw std::runtime_error("stable_vector constructing chunk");
      return 0;
    }
  }
  static Chunk clone_chunk(Chunk from) {
    Chunk c = (Chunk)::operator new(sizeof(T)*chunksize);
    I i = 0;
    try {
      for (; i < chunksize; ++i)
        new (&c[i]) T(from[i]);
      return c;
    } catch (...) {
      while (i)
        c[--i].~T();
      ::operator delete((void*)c);
      throw std::runtime_error("stable_vector constructing chunk");
      return 0;
    }
  }

  void push_back_chunk(Chunk c) {
    chunks.push_back(c);
    capacity += chunksize;
  }

  static void free_chunk(Chunk c) {
    I i = chunksize;
    while (i)
      c[--i].~T();
    ::operator delete((void*)c);
  }
};

}//ns

template <class T>
T& grow(std::vector<T> &v, std::size_t i) {
  if (i >= v.size())
    v.resize(i+1);
  return v[i];
}

template <class T>
void clear_destroy(std::vector<T> &v) {
  std::vector<T> destroy;
  v.swap(destroy);
}

template <class T>
void shrink_destroy(std::vector<T> &v, std::size_t sz) {
  std::size_t oldsz = v.size();
  if (sz < oldsz) {
    if (!sz)
      clear_destroy(v);
    v.resize(sz);
  }
}

#ifndef _WIN32
#ifdef GRAEHL_TEST

#include <graehl/shared/test.hpp>
#include <graehl/shared/small_vector.hpp>

namespace graehl { namespace test {

typedef stable_vector<int, std::size_t, 8, true> StableVector1;
typedef stable_vector<std::size_t, unsigned, 2, false> StableVector2;
typedef std::vector<std::size_t> Vector3;

template <class T>
inline void test_stable_vector(bool isStable = true) {
  test_small_vector_1<T>();
  T v;
  typename T::value_type & v1333 = grow(v, 1333);
  v1333 = 1337;
  BOOST_REQUIRE_EQUAL(v.size(), 1333 + 1);
  BOOST_CHECK_EQUAL(v[1332], 0);
  BOOST_CHECK_EQUAL(v[1333], 1337);
  BOOST_CHECK_EQUAL(&v[1333], &v1333);
  v.resize(9999);
  grow(v, 99999);
  BOOST_REQUIRE_EQUAL(v.size(), 99999 + 1);
  BOOST_CHECK_EQUAL(v[1333], 1337);
  if (isStable)
    BOOST_CHECK_EQUAL(&v[1333], &v1333);
}

BOOST_AUTO_TEST_CASE(stable_vector_test_case) {
  test_stable_vector<StableVector1>();
  test_stable_vector<StableVector2>();
  test_stable_vector<Vector3>(false);
}

}}

#endif
#endif

#endif
