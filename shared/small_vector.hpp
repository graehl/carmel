#ifndef GRAEHL_SHARED___SMALL_VECTOR
#define GRAEHL_SHARED___SMALL_VECTOR


/** \file

    space and cache-efficient small vectors (like std::vector). for small size,
    elements are stored without indirection on the stack. over that size,
    they're stored on the heap as with a regular std::vector. specify the
    maximum 'small size' as a template constant. also specify an alternative to
    std::size_t (e.g. uint32_t or uint16_t) for element indices and sizes.

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

    TODO: need to implement tests/handling for non-pod (guarantee ctor/dtor
    calls in that case, incl copy ctor when changing between heap and inline). for now assume that T must be POD.

    REQUIRES that T is POD (specifically - it's part of a union, so must be. all
    we really require is that memmove/memcpy is allowed rather than copy
    construct + destroy old

    memcmp comparable isn't necessary - we use ==, <.

    you can check for the happy day when non-pod small_vector are supported
    with: #if !GRAEHL_SMALL_VECTOR_POD_ONLY - it's just some extra bookkeeping
    and traits usage to meticulously call copy ctor and dtor instead of memcpy
    and nothing respectively (assuming the compiler allows your non-pod to exist
    in a union)

*/

#define GRAEHL_SMALL_VECTOR_POD_ONLY 1

#include <stdlib.h>

/**
   if 1, complete valgrind safety at the cost of speed. if 0, code is still
   correct but gives spurious use of uninitialized value on copying inlined
   vectors
*/

#ifndef GRAEHL_VALGRIND
# if NDEBUG
#  define GRAEHL_VALGRIND 0
# else
#  define GRAEHL_VALGRIND 1
# endif
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
# define GRAEHL_SMALL_VECTOR_SERIALIZE_VERSION 0
#endif
#if GRAEHL_SMALL_VECTOR_SERIALIZE_VERSION
#include <boost/serialization/version.hpp>
#include <boost/serialization/item_version_type.hpp>
#endif

#define GRAEHL_BOOST_SERIALIZATION_NVP(v) BOOST_SERIALIZATION_NVP(v)
#define GRAEHL_BOOST_SERIALIZATION_NVP_VERSION(v) BOOST_SERIALIZATION_NVP(v)


#include <iterator>
#include <functional>
#include <boost/serialization/array.hpp>
#include <boost/serialization/wrapper.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/collection_size_type.hpp>
#include <boost/assign/list_inserter.hpp>
#include <boost/range/iterator_range.hpp>
#include <algorithm>
#include <cstring>
#include <cassert>
#include <boost/cstdint.hpp>
#include <new>
#include <boost/functional/hash.hpp>
#include <graehl/shared/swap_pod.hpp>
#include <graehl/shared/word_spacer.hpp>

namespace graehl {

template <class IterTraits>
struct enable_type {
  typedef void type;
};

// recommend an odd number for kMaxInlineSize on 64-bit (check sizeof to see
// that you get the extra element for free)
template <class T,unsigned kMaxInlineSize=3,class Size=unsigned>
struct small_vector {
  typedef small_vector Self;
  typedef Size size_type;

  /**
     may leak (if you don't free yourself)
  */
  void clear_nodestroy() {
    data.stack.sz_ = 0;
  }

#if __cplusplus >= 201103L || CPP11
  small_vector(small_vector && o)
  {
    std::memcpy(this, &o, sizeof(small_vector));
    o.clear_nodestroy();
  }

  small_vector & operator=(small_vector && o)
  {
    if (&o != this) {
      free();
      std::memcpy(this, &o, sizeof(small_vector));
      o.clear_nodestroy();
    }
    return *this;
  }
#endif

  BOOST_SERIALIZATION_SPLIT_MEMBER()
  template <class Archive>
  void save(Archive & ar, const unsigned int) const {
    using namespace boost::serialization;
    collection_size_type const count(data.stack.sz_);
    ar << GRAEHL_BOOST_SERIALIZATION_NVP(count);
#if GRAEHL_SMALL_VECTOR_SERIALIZE_VERSION
    item_version_type const item_version(version<T>::value);
    ar << GRAEHL_BOOST_SERIALIZATION_NVP_VERSION(item_version); // for text archive only apparently? use_array_optimization only fires for binary archive?
#endif
    if (data.stack.sz_)
      ar << GRAEHL_BOOST_SERIALIZATION_NVP(make_array(begin(),data.stack.sz_));
  }
  template<class Archive>
  void load(Archive & ar, const unsigned int) {
    using namespace boost::serialization;
    collection_size_type count;
    ar >> GRAEHL_BOOST_SERIALIZATION_NVP(count);
    resize((size_type)count);
    assert(count==data.stack.sz_);
#if GRAEHL_SMALL_VECTOR_SERIALIZE_VERSION
    item_version_type version;
    ar >> GRAEHL_BOOST_SERIALIZATION_NVP_VERSION(version);
#endif
    if (data.stack.sz_)
      ar >> GRAEHL_BOOST_SERIALIZATION_NVP(make_array(begin(),data.stack.sz_));
  }

  small_vector() {
    data.stack.sz_ = 0;
  }
  /**
     default constructed T() * s. since T is probably POD, this means its pod
     members are default constructed (i.e. set to 0). If T isn't POD but can be memmoved, then you probably really wanted the constructor called.
  */
  explicit small_vector(size_type s) {
    assert(s<=kMaxSize);
    alloc(s);
    if (s <= kMaxInlineSize)
      for (size_type i = 0; i < s; ++i) new(&data.stack.vals_[i]) T();
    else
      for (size_type i = 0; i < data.stack.sz_; ++i) new(&data.heap.begin_[i]) T();
  }

  small_vector(int s, T const& v) {
    assert(s>=0);
    init((size_type)s,v);
  }

#ifndef XMT_32 // avoid signature collision
  small_vector(unsigned s, T const& v) {
    init((size_type)s,v);
  }
#endif

  small_vector(std::size_t s, T const& v) {
    init((size_type)s,v);
  }

  template <class Val>
  small_vector(int s, Val const& v) {
    assert(s>=0);
    init((size_type)s,v);
  }

#ifndef XMT_32 // avoid signature collision
  template <class Val>
  small_vector(unsigned s, Val const& v) {
    init((size_type)s,v);
  }
#endif

  template <class Val>
  small_vector(std::size_t s, Val const& v) {
    init((size_type)s,v);
  }


  small_vector(T const* i,T const* end) {
    init_range(i,end);
  }

  template <class Iter>
  small_vector(Iter const& i,Iter const& end
               , typename enable_type<typename Iter::value_type>::type *enable=0)
      // couldn't SFINAE on std::iterator_traits<Iter> in gcc (for Iter=int)
  {
    init_range(i,end);
  }

 protected:
  void init(size_type s, T const& v) {
    assert(s<=kMaxSize);
    alloc(s);
    if (s <= kMaxInlineSize) {
      for (size_type i = 0; i < s; ++i) data.stack.vals_[i] = v;
    } else {
      for (size_type i = 0; i < data.stack.sz_; ++i) data.heap.begin_[i] = v;
    }
  }
  void init_range(T const* i,T const* end) {
    size_type s=(size_type)(end-i);
    assert(s<=kMaxSize);
    alloc(s);
    memcpy_from(i);
  }
  template <class I>
  void init_range(I const& i,I const& end) {
    size_type s=(size_type)std::distance(i,end);
    assert(s<=kMaxSize);
    alloc(s);
    std::copy(i,end,begin());
  }
 public:

  /**
     copy ctor.
  */
  small_vector(small_vector const& o) {
    if (o.data.stack.sz_ <= kMaxInlineSize) {
#if GRAEHL_VALGRIND
      data.stack.sz_=o.data.stack.sz_;
      for (size_type i=0;i<data.stack.sz_;++i) data.stack.vals_[i]=o.data.stack.vals_[i];
#else
      data.stack=o.data.stack;
#endif
      //TODO: valgrind for partial init array
    } else {
      data.heap.capacity_ = data.heap.sz_ = o.data.heap.sz_;
      alloc_heap();
      memcpy_heap(o.data.heap.begin_);
    }
  }

  template <class Alloc>
  small_vector(std::vector<T,Alloc> const& vec) {
    init(&*vec.begin(),&*vec.end());
  }

  typedef T const* const_iterator;
  typedef T* iterator;
  typedef T value_type;
  typedef T &reference;
  typedef T const& const_reference;
  static const size_type kMaxSize=
#ifdef NDEBUG
      (size_type)-1;
#else
  ((size_type)-1)/2;
#endif
  static const size_type kInitHeapSize=kMaxInlineSize*2;
  T *begin() { return data.stack.sz_ > kMaxInlineSize ? data.heap.begin_ : data.stack.vals_; }
  T const* begin() const { return const_cast<small_vector*>(this)->begin(); }
  T *end() { return begin() + data.stack.sz_; }
  T const* end() const { return begin() + data.stack.sz_; }
  typedef std::pair<T const*, T const*> slice_type;
  slice_type slice() const {
    return data.stack.sz_ > kMaxInlineSize ?
        slice_type(data.heap.begin_, data.heap.begin_ + data.stack.sz_) :
        slice_type(data.stack.vals_, data.stack.vals_ + data.stack.sz_);
  }

  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
  const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }

  //TODO: test.  this invalidates more iterators than std::vector since resize may move from ptr to vals.
  T *erase(T *b) {
    return erase(b,b+1);
  }
  T *erase(T *b,T* e) { // remove [b,e) and return pointer to element e
    T *tb=begin(),*te=end();
    size_type nbefore=(size_type)(b-tb);
    if (e==te) {
      resize(nbefore);
    } else {
      size_type nafter=(size_type)(te-e);
      std::memmove(b,e,nafter*sizeof(T));
      // memmove instead of memcpy for case when someone erases middle of vector and so source, destination ranges overlap
      resize(nbefore+nafter);
    }
    return begin()+nbefore;
  }
  void memcpy_to(T *p) {
    std::memcpy(p,begin(),data.stack.sz_*sizeof(T));
  }
  void memcpy_heap(T const* from) {
    assert(data.stack.sz_<=kMaxSize);
    assert(data.heap.capacity_>=data.stack.sz_);
    std::memcpy(data.heap.begin_,from,data.stack.sz_*sizeof(T));
  }
  void memcpy_from(T const* from) {
    assert(data.stack.sz_<=kMaxSize);
    std::memcpy(this->begin(),from,data.stack.sz_*sizeof(T));
  }
  static inline void memcpy_n(T *to,T const* from,size_type n) {
    std::memcpy(to,from,n*sizeof(T));
  }
  static inline void memmove_n(T *to,T const* from,size_type n) {
    std::memmove(to,from,n*sizeof(T));
  }

  small_vector & operator=(small_vector const& o) {
    if (&o == this) return *this;
    if (data.stack.sz_ <= kMaxInlineSize) {
      if (o.data.stack.sz_ <= kMaxInlineSize) {
        data.stack.sz_ = o.data.stack.sz_;
        // unsigned instead of size_type because kMaxInlineSize is unsigned
        for (unsigned i = 0; i < kMaxInlineSize; ++i) data.stack.vals_[i] = o.data.stack.vals_[i];
      } else {
        data.heap.capacity_ = data.stack.sz_ = o.data.stack.sz_;
        alloc_heap();
        memcpy_heap(o.data.heap.begin_);
      }
    } else {
      if (o.data.stack.sz_ <= kMaxInlineSize) {
        free_heap();
        data.stack.sz_ = o.data.stack.sz_;
        for (unsigned i = 0; i < data.stack.sz_; ++i) data.stack.vals_[i] = o.data.stack.vals_[i];
      } else {
        if (data.heap.capacity_ < o.data.stack.sz_) {
          free_heap();
          data.heap.capacity_ = o.data.stack.sz_;
          alloc_heap();
        }
        data.stack.sz_ = o.data.stack.sz_;
        memcpy_heap(o.data.heap.begin_);
      }
    }
    return *this;
  }

  ~small_vector() {
    free();
  }

  void clear() {
    free();
    data.stack.sz_ = 0;
  }

  void assign(T const* i,T const* end) {
    size_type n=(size_type)(end-i);
    memcpy_n(realloc(n),i,n);
  }
  template <class I>
  void assign(I i,I end) {
    T *o=realloc((size_type)std::distance(i,end));
    for (;i!=end;++i)
      *o++=*i;
  }
  template <class Set>
  void assign(Set const& set) {
    assign(set.begin(),set.end());
  }
  template <class I>
  void assign_input(I i,I end) {
    clear();
    for(;i!=end;++i)
      push_back(*i);
  }
  void assign(size_type n, T const& v) {
    free();
    init(n, v);
  }


  bool empty() const { return data.stack.sz_ == 0; }
  size_type size() const { return data.stack.sz_; }
  size_type capacity() const { return data.stack.sz_>kMaxInlineSize ? data.heap.capacity_ : data.stack.sz_; }
  // our true capacity is never less than kMaxInlineSize, but since capacity talks about allocated space,
  // we may as well return 0; however, docs on std::vector::capacity imply capacity>=size.

  /// does not initialize w/ default ctor.
  inline void resize_up_unconstructed(size_type s) { // like reserve, but because of capacity_ undef if data.stack.sz_<=kMaxInlineSize, must set data.stack.sz_ immediately or we lose invariant
    assert(s>=data.stack.sz_);
    if (s>kMaxInlineSize)
      reserve_up_big(s);
    data.stack.sz_ = s;
  }
  void resize_up_big_unconstructed(size_type s) {
    assert(s>kMaxInlineSize);
    reserve_up_big(s);
    data.stack.sz_ = s;
  }
  void reserve(size_type s) {
    if (data.stack.sz_ > kMaxInlineSize)
      reserve_up_big(s);
  }


  // aliasing warning: if append is within self, undefined result (because we invalidate iterators as we grow)
  /**
     append the range [i,end) without being able to forecast the size in advance:
  */
  template <class InputIter>
  inline void append_input(InputIter i,InputIter end) {
    if (data.small.sz_<=kMaxInlineSize) {
      for (;data.small.sz_<kMaxInlineSize;++data.small.sz_) {
        data.small.vals_[data.small.sz_]=*i;
        ++i;
        if (i==end) return;
      }
      copy_vals_to_ptr();
      data.large.begin_[kMaxInlineSize]=*i;
      ++i;
      data.small.sz_=kMaxInlineSize+1;
    }
    for (;i!=end;++i)
      push_back_large(*i);
  }

  template <class ForwardIter>
  inline void append(ForwardIter i,ForwardIter e) {
    size_type N=(size_type)std::distance(i,e);
    size_type s=data.stack.sz_;
    size_type addsz=(size_type)(e-i);
    resize_up_unconstructed(s+addsz);
    for (T *b=begin()+s;i<e;++i,++b)
      *b=*i;
  }

  inline void append(T const* i, size_type n) {
    size_type s=data.stack.sz_;
    append_unconstructed(n);
    memcpy_n(begin()+s,i,n);
  }

  inline void append(T const* i,T const* end) {
    size_type s=data.stack.sz_;
    size_type n=(size_type)(end-i);
    append_unconstructed(n);
    memcpy_n(begin()+s,i,n);
  }

  template <class Set>
  inline void append(Set const& set) {
    append(set.begin(),set.end());
  }

  inline void append(small_vector const& set) {
    append(set.begin(), set.data.stack.sz_);
  }

  inline void insert(T const& v) {
    push_back(v);
  }

  /**
     increase size without calling default ctor.
  */
  inline void append_unconstructed(size_type N) {
    resize_up_unconstructed(data.stack.sz_+N);
  }

  /**
     change size without calling default ctor.
  */
  inline void resize_unconstructed(size_type N) {
    if (data.stack.sz_ > N)
      resize(N);
    else if (data.stack.sz_ < N)
      resize_up_unconstructed(N);
  }

  /**
     insert hole of N elements at iterator i
  */
  inline void insert_hole(iterator where, size_type N) {
    if (where==end())
      append_unconstructed(N);
    else
      insert_hole_index(where-begin(),N);
  }

  /**
     insert hole of N elements at index i.
  */
  inline T *insert_hole_index(size_type i, size_type N) {
    size_type s=data.stack.sz_;
    size_type snew=s+N;
    resize_up_unconstructed(snew);
    //TODO: optimize for snew,s inline/not
    T *v=begin();
    memmove_n(v+i+N,v+i,s-i);
    return v+i;
  }

  template <class ForwardIter>
  inline void insert(iterator where, ForwardIter i, ForwardIter e) {
    if (where==end())
      append(i,e);
    else
      insert_index((size_type)(where-begin()),i,e);
  }

  inline void insert(iterator where, T const* i, T const* e) {
    if (where==end())
      append(i,e);
    else
      insert_index((size_type)(where-begin()),i,e);
  }

  template <class ForwardIter>
  inline void insert_index(size_type atIndex, ForwardIter i, ForwardIter e) {
    size_type N=(size_type)std::distance(i,e);
    T *o=insert_hole_index(atIndex,N);
    for (;i!=e;++i,++o)
      *o=*i;
  }

  inline void insert_index(size_type atIndex, T const* i, T const* e) {
    size_type N=(size_type)(e-i);
    memcpy_n(insert_hole_index(atIndex,N),i,N);
  }

  inline void insert_index(size_type where, T const& t) {
    memcpy_n(insert_hole_index(where,1),&t,1);
  }

  inline void insert(iterator where, T const& t) {
    insert_index((size_type)(where-begin()),t);
  }

  /**
     O(n) of course.
  */
  inline void push_front(T const& t) {
    insert_index(0,t);
  }

  inline void insert_index(size_type where, size_type n, T const& t) {
    insert_hole_index(where,n);
    T *o=begin()+where;
    while(--n) *o++=t;
  }

  inline void insert_index(iterator where, size_type n, T const& t) {
    insert_index(where-begin(),n,t);
  }

  inline void push_back_heap(T const& v) {
    if (data.stack.sz_ == data.heap.capacity_)
      ensure_capacity_grow(data.stack.sz_+1);
    data.heap.begin_[data.stack.sz_++] = v;
  }

  inline void push_back(T const& v) {
    if (data.stack.sz_ < kMaxInlineSize) {
      data.stack.vals_[data.stack.sz_] = v;
      ++data.stack.sz_;
    } else if (data.stack.sz_ == kMaxInlineSize) {
      copy_vals_to_ptr();
      data.heap.begin_[kMaxInlineSize]=v;
      data.stack.sz_=kMaxInlineSize+1;
    } else {
      push_back_heap(v);
    }
  }

  T& back() { return this->operator[](data.stack.sz_ - 1); }
  const T& back() const { return this->operator[](data.stack.sz_ - 1); }
  T& front() { return this->operator[](0); }
  const T& front() const { return this->operator[](0); }

  void pop_back() {
    assert(data.stack.sz_>0);
    --data.stack.sz_;
    if (data.stack.sz_==kMaxInlineSize)
      ptr_to_small();
  }

  /**
     free unused (heap) space.
  */
  void compact() {
    compact(data.stack.sz_);
  }

  /**
     shrink (newsz must be <= size()) and free unused (heap) space.
  */
  void compact(size_type newsz) {
    assert(newsz<=data.stack.sz_);
    if (data.stack.sz_>kMaxInlineSize) { // was heap
      data.stack.sz_=newsz;
      if (newsz<=kMaxInlineSize) // now small
        ptr_to_small();
    } else // was small already
      data.stack.sz_=newsz;
  }

  void resize(size_type s, T const& v = T()) {
    assert(s<=kMaxSize);
    assert(data.stack.sz_<=kMaxSize);
    if (s <= kMaxInlineSize) {
      if (data.stack.sz_ > kMaxInlineSize) {
        data.stack.sz_ = s;
        ptr_to_small();
        return;
      } else if (s<=data.stack.sz_) {
      } else { // growing but still small
        for (size_type i = data.stack.sz_; i < s; ++i)
          data.stack.vals_[i] = v;
      }
    } else { // new s is heap
      if (s > data.stack.sz_) {
        reserve_up_big(s);
        for (size_type i = data.stack.sz_;i<s;++i)
          data.heap.begin_[i] = v;
      }
    }
    data.stack.sz_ = s;
  }

  T& operator[](size_type i) {
    if (data.stack.sz_ <= kMaxInlineSize) return data.stack.vals_[i];
    return data.heap.begin_[i];
  }
  T const& operator[](size_type i) const {
    if (data.stack.sz_ <= kMaxInlineSize) return data.stack.vals_[i];
    return data.heap.begin_[i];
  }

  bool operator==(small_vector const& o) const {
    if (data.stack.sz_ != o.data.stack.sz_) return false;
    if (data.stack.sz_ <= kMaxInlineSize) {
      for (size_type i = 0; i < data.stack.sz_; ++i)
        if (data.stack.vals_[i] != o.data.stack.vals_[i]) return false;
      return true;
    } else {
      for (size_type i = 0; i < data.stack.sz_; ++i)
        if (data.heap.begin_[i] != o.data.heap.begin_[i]) return false;
      return true;
    }
  }

  bool operator==(std::vector<T> const& other) const {
    if (data.stack.sz_ != other.size()) return false;
    if (data.stack.sz_ <= kMaxInlineSize) {
      for (size_type i = 0; i < data.stack.sz_; ++i)
        if (data.stack.vals_[i] != other[i]) return false;
      return true;
    } else {
      for (size_type i = 0; i < data.stack.sz_; ++i)
        if (data.heap.begin_[i] != other[i]) return false;
      return true;
    }
  }


  friend bool operator!=(small_vector const& a, small_vector const& b) {
    return !(a==b);
  }

  /**
     shortest-first total ordering (not lex.)
  */
  bool operator<(small_vector const& o) const {
    return compare_by_less<bool,true,false,false>(o);
  }

  /**
     shortest-first total ordering (not lex.)
  */
  bool operator<=(small_vector const& o) const {
    return compare_by_less<bool,true,true,false>(o);
  }

  /**
     shortest-first total ordering (not lex.)
  */
  bool operator>=(small_vector const& o) const {
    return compare_by_less<bool,false,true,true>(o);
  }

  /**
     shortest-first total ordering (not lex.)
  */
  bool operator>(small_vector const& o) const {
    return compare_by_less<bool,false,false,true>(o);
  }

  /**
     using T::operator<, return this <=> o :

     *this == o:  0.
     *this < o: -1
     *this > o: 1
     */
  int compare(small_vector const& o) const {
    return compare_by_less<int,-1,0,1>(o);
  }

  /**
     return iterator for matching element, if any, else end
  */
  template <class Target>
  T *find(Target const& target) const {
    return std::find(begin(),end(),target);
  }

  /** \return this->find(target)!=this->end() but might optimize better
   */
  template <class Target>
  bool contains(Target const& target) const {
    for (const_iterator i=begin(),e=end();i!=e;++i)
      if (*i==target) return true;
    return false;
  }

  template <class Target>
  friend inline bool contains(small_vector const& vec,Target const& target) { return vec.contains(target); }

  typedef boost::iterator_range<iterator> iterator_range;
  typedef boost::iterator_range<const_iterator> const_iterator_range;

  iterator_range range() { return data.stack.sz_>kMaxInlineSize ?
        iterator_range(data.heap.begin_,data.heap.begin_+data.stack.sz_) :
        iterator_range(data.stack.vals_,data.stack.vals_+data.stack.sz_);
  }
  const_iterator_range range() const { return data.stack.sz_>kMaxInlineSize ?
        const_iterator_range(data.heap.begin_,data.heap.begin_+data.stack.sz_) :
        const_iterator_range(data.stack.vals_,data.stack.vals_+data.stack.sz_);
  }

  void swap(small_vector& o) {
    assert(this);
    assert(&o);
    swap_pod(*this,o);
  }
  friend inline void swap(small_vector &a,small_vector &b) {
    return a.swap(b);
  }

  inline std::size_t hash_impl() const {
    using namespace boost;
    return (data.stack.sz_ <= kMaxInlineSize) ?
        hash_range(data.stack.vals_,data.stack.vals_+data.stack.sz_) :
        hash_range(data.heap.begin_,data.heap.begin_+data.stack.sz_);
  }

  /**
     \return small_vector<..>(begin,end).hash_impl() without actually creating a
     copy of the substring
  */
  static inline std::size_t hash_substr(const_iterator begin,const_iterator end) {
    using namespace boost;
    return hash_range(begin,end);
  }

  // for boost::hash<>
  friend inline std::size_t hash_value(small_vector const& x) { return x.hash_impl(); }

  template <class Out>
  void print(Out &o) const {
    range_sep().print(o,begin(),end());
  }
  template <class Ch,class Tr>
  friend std::basic_ostream<Ch,Tr>& operator<<(std::basic_ostream<Ch,Tr> &o, small_vector const& self)
  { self.print(o); return o; }
  void init_unconstructed(size_type s) {
    assert(data.stack.sz_ == 0);
    alloc(s);
  }
 private:
  void alloc(size_type s) { // doesn't free old; for ctor. sets sz_
    data.stack.sz_ = s;
    assert(s <= kMaxSize);
    if (s>kMaxInlineSize) {
      data.heap.capacity_ = s;
      alloc_heap();
    }
  }
  static inline T* alloc_impl(size_type sz) {
    return (T*)std::malloc(sizeof(T)*sz);
  }
  static void free_impl(T *alloced) {
    std::free(alloced);
  }
  //pre: capacity_ is set
  void alloc_heap() {
    data.heap.begin_ = alloc_impl(data.heap.capacity_);  // TODO: boost user_allocator static template
  }
  void free_heap() const {
    free_impl(data.heap.begin_);
  }
  void free() {
    if (data.stack.sz_ > kMaxInlineSize)
      free_heap();
  }
  T *realloc(size_type s) {
    if (s==data.stack.sz_)
      return begin();
    free();
    data.stack.sz_ = s;
    if (s>kMaxInlineSize) {
      data.heap.capacity_=s;
      alloc_heap();
      return data.heap.begin_;
    } else
      return data.stack.vals_;
  }


  void reserve_up_big(size_type s) {
    assert(s>kMaxInlineSize);
    if (data.stack.sz_>kMaxInlineSize) {
      if (s>data.heap.capacity_)
        ensure_capacity_grow(s);
    } else {
      T* tmp = alloc_impl(s);
      memcpy_n(tmp, data.stack.vals_, data.stack.sz_);
      data.heap.capacity_ = s; // note: these must be set AFTER copying from data.stack.vals_
      data.heap.begin_ = tmp;
    }
  }
  void ensure_capacity(size_type min_size) {
    // only call if you're already heap
    assert(min_size > kMaxInlineSize);
    assert(data.stack.sz_ > kMaxInlineSize);
    if (min_size < data.heap.capacity_) return;
    ensure_capacity_grow(min_size);
  }
  void ensure_capacity_grow(size_type min_size) {
#ifdef _MSC_VER
# undef max
#endif
    size_type new_cap = std::max(static_cast<size_type>(data.heap.capacity_ * 2), min_size);
    T* tmp = alloc_impl(new_cap);
    memcpy_n(tmp, data.heap.begin_, data.stack.sz_);
    free_heap();
    data.heap.begin_ = tmp;  // note: these must be set AFTER copying from old vals
    data.heap.capacity_ = new_cap; // set after free_heap (though current allocator doesn't need old capacity)
  }


  void copy_vals_to_ptr() {
    assert(data.stack.sz_<=kMaxInlineSize);
    T* newHeapVals = alloc_impl(kInitHeapSize); //note: must use tmp to not destroy data.stack.vals_
    memcpy_n(newHeapVals, data.stack.vals_,
#if GRAEHL_VALGRIND
             data.stack.sz_
#else
             kMaxInlineSize // may copy more than actual size_. ok. constant should be more optimizable
#endif
             );
    ;
    data.heap.capacity_ = kInitHeapSize;
    data.heap.begin_=newHeapVals;
    // only call if you're going to immediately increase size_ to >kMaxInlineSize
  }
  void ptr_to_small() {
    assert(data.stack.sz_<=kMaxInlineSize); // you decreased size_ already. normally ptr wouldn't be used if size_ were this small
    T *fromHeap=data.heap.begin_; // should be no problem with memory access order (strict aliasing) because of safe access through union
    for (size_type i=0;i<data.stack.sz_;++i) // no need to memcpy for small size
      data.stack.vals_[i]=fromHeap[i]; // note: it was essential to save fromHeap first because data.stack.vals_ union-competes
    free_impl(fromHeap);
  }

  // o is longer. so if equal at end, then kLess.
  template <class Ret,int kLess,int kGreater>
  inline Ret compare_by_less_len_differs(small_vector const& o) {
    const_iterator e=end();
    std::pair<const_iterator,const_iterator> mo=std::mismatch(begin(),e,o.begin());
    if (mo.first==e) return kLess;
    return *mo.first<*mo.second ? kLess : kGreater;
  }
  //e.g. <: valLess=true, others = false. 3-value compare: Ret=int,kLess=-1,kGreater=1,kEqual=0
  //      (an arbitrary total ordering (not lex.))
  template <class Ret,int kLess,int kEqual,int kGreater>
  inline Ret compare_by_less(small_vector const& o) const {
    if (data.stack.sz_ == o.data.stack.sz_) {
      if (data.stack.sz_ <= kMaxInlineSize) {
        for (size_type i = 0; i < data.stack.sz_; ++i) {
          if (data.stack.vals_[i] < o.data.stack.vals_[i]) return kLess;
          if (o.data.stack.vals_[i] < data.stack.vals_[i]) return kGreater;
        }
        return kEqual;
      } else {
        for (size_type i = 0; i < data.stack.sz_; ++i) {
          if (data.heap.begin_[i] < o.data.heap.begin_[i]) return kLess;
          if (o.data.heap.begin_[i] < data.heap.begin_[i]) return kGreater;
        }
        return kEqual;
      }
    }
    return std::lexicographical_compare(begin(),end(),o.begin(),o.end()) ? kLess : kGreater;
    //return (data.stack.sz_ < o.data.stack.sz_) ? kLess : kGreater; // faster but not lexicograph.
  }


  /* guarantee from c++ standard:

     one special guarantee is made in order to simplify the use of unions: If a POD-union contains several POD-structs that share a common initial sequence (9.2), and if an object of this POD-union type contains one of the POD-structs, it is permitted to inspect the common initial sequence of any of POD-struct members; see 9.2. ]
  */
  // anything that shrinks the array may mean copying back and forth, e.g. using small_vector as a stack with repeated push/pop over and back to kMaxInlineSize
  /**
     this union forms the full contents of small_vector. it's probably best if kMaxInlineSize is at least 2 (unless T is *extremely* large compared to size_type). you can experiment by checking sizeof(small_vector<...>) - the idea would be not to use the smaller of two kMaxInlineSize that both result in the same size union
  */
  union storage_union_variants {
    struct heap_storage_variant {
      size_type sz_;  // common prefix with small_storage.sz_
      size_type capacity_;  // only initialized when size_ > kMaxInlineSize. must not initialize when copying from small.vals_ until after all are copied.
      T* begin_; // otherwise
    } heap;
    struct inline_storage_variant {  // only initialized when size_ <= kMaxInlineSize
      size_type sz_;
      T vals_[kMaxInlineSize]; // iff size_<=kMaxInlineSize (note: tricky!
    } stack;
    // note: we have a shared initial sz_ member in each to ensure the best alignment/padding.
    // (otherwise it would be simpler to move sz_ outside the union
  };
  storage_union_variants data;
};

}//ns

namespace std {
template <class V,unsigned MaxInline,class Size>
inline void swap(graehl::small_vector<V,MaxInline,Size> &a,graehl::small_vector<V,MaxInline,Size> &b) {
  a.swap(b);
}

#ifdef _MSC_VER
# pragma warning(disable:4099)
// msvc has class hash. should be struct. this hides warning.
#endif

template <class T>
struct hash;

template <class V,unsigned MaxInline,class Size>
struct hash<graehl::small_vector<V,MaxInline,Size> > {
 public:
  std::size_t operator()(graehl::small_vector<V,MaxInline,Size> const& vec) const {
    return vec.hash_impl();
  }
};

}

namespace boost {
namespace assign {

template <class V,unsigned MaxInline,class Size,class V2>
inline list_inserter<assign_detail::call_push_back<graehl::small_vector<V,MaxInline,Size> >, V>
operator+=(graehl::small_vector<V,MaxInline,Size> & c,V2 v)
{
  return push_back(c)(v);
}

}

namespace serialization {

template <class V,unsigned MaxInline,class Size>
struct implementation_level<graehl::small_vector<V,MaxInline,Size> >
{
  typedef mpl::int_<object_serializable> type;
  typedef mpl::integral_c_tag tag;
  BOOST_STATIC_CONSTANT(int,value=implementation_level::type::value);
};

/**
   since small_vector is a template,
   http://www.boost.org/doc/libs/1_48_0/libs/serialization/doc/traits.html#level
   recommends this to disable version tracking. note that object_serializable
   applies by default.
*/
template <class V,unsigned MaxInline,class Size>
struct tracking_level<graehl::small_vector<V,MaxInline,Size> >
{
  typedef mpl::int_<track_never> type;
  typedef mpl::integral_c_tag tag;
  BOOST_STATIC_CONSTANT(int,value=tracking_level::type::value);
};

}
}//ns


#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <sstream>

#define EXPECT_EQ_NOPRINT(x,y) BOOST_CHECK((x)==(y))

namespace graehl { namespace test {

std::size_t const test_archive_flags=boost::archive::no_header|boost::archive::no_codecvt|boost::archive::no_xml_tag_checking;

template <class InArchive,class Vec1,class Vec2>
void test_same_serialization_result(std::string const& str,Vec1 const& v, Vec2 &v2) {
  std::stringstream ss(str);
  InArchive ia(ss,test_archive_flags);
  ia >> v2;
  BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(),v.end(),v2.begin(),v2.end());
}

template <class Archive,class InArchive,class Vec1,class Vec2>
void test_same_serialization(Vec1 const& v, Vec2 const& v2) {
  Vec1 vb;
  Vec2 v2b;
  using namespace std;
  stringstream ss,ss2;
  Archive oa(ss,test_archive_flags),oa2(ss2,test_archive_flags);
  oa << v;
  oa2 << v2;
  string s=ss.str(),s2=ss2.str();
  BOOST_REQUIRE_EQUAL(s,s2);
  test_same_serialization_result<InArchive>(s,v,vb);
  EXPECT_EQ_NOPRINT(vb,v);
  test_same_serialization_result<InArchive>(s,v,v2b);
  EXPECT_EQ_NOPRINT(v2b,v);
  test_same_serialization_result<InArchive>(s2,v2,vb);
  EXPECT_EQ_NOPRINT(v2,vb);
  test_same_serialization_result<InArchive>(s2,v2,v2b);
  EXPECT_EQ_NOPRINT(v2,v2b);
}

template <class SmallVecInt,class Archive,class InArchive>
void test_same_serialization(std::vector<int> const& v) {
  SmallVecInt sv(v.begin(),v.end());
  test_same_serialization<Archive,InArchive>(v,sv);
}

template <class SmallVecInt>
void test_same_serializations(std::vector<int> const& v) {
#if GRAEHL_SMALL_VECTOR_SERIALIZE_VERSION
  test_same_serialization<SmallVecInt,boost::archive::text_oarchive,boost::archive::text_iarchive>(v);
#else
  test_same_serialization<SmallVecInt,boost::archive::binary_oarchive,boost::archive::binary_iarchive>(v);
#endif
}

template <class SmallVecInt>
void test_small_vector_same_serializations()
{
  std::vector<int> v;
  //  test_same_serializations(v);
  for (int i=10;i<19;++i) {
    v.push_back(i);
    test_same_serializations<SmallVecInt>(v);
  }
}

template <class VectorInt>
void test_small_vector_1()
{
  using namespace std;
  VectorInt v;
  VectorInt v2;
  v.push_back(0);
  v.push_back(1);
  EXPECT_EQ(v[1],1);
  EXPECT_EQ(v[0],0);
  v.push_back(2);
  EXPECT_EQ(v.size(),3u);
  EXPECT_EQ(v[2],2);
  EXPECT_EQ(v[1],1);
  EXPECT_EQ(v[0],0);
  v2 = v;
  VectorInt copy(v);
  EXPECT_EQ(copy.size(),3u);
  EXPECT_EQ(copy[0],0);
  EXPECT_EQ(copy[1],1);
  EXPECT_EQ(copy[2],2);
  EXPECT_EQ_NOPRINT(copy,v2);
  copy[1] = 99;
  EXPECT_TRUE(copy != v2);
  EXPECT_EQ(v2.size(),3);
  EXPECT_EQ(v2[2],2);
  EXPECT_EQ(v2[1],1);
  EXPECT_EQ(v2[0],0);
  v2[0] = -2;
  v2[1] = -1;
  v2[2] = 0;
  EXPECT_EQ(v2[2],0);
  EXPECT_EQ(v2[1],-1);
  EXPECT_EQ(v2[0],-2);
  VectorInt v3(1,1);
  EXPECT_EQ(v3[0],1);
  v2 = v3;
  EXPECT_EQ(v2.size(),1);
  EXPECT_EQ(v2[0],1);
  VectorInt v4(10, 1);
  EXPECT_EQ(v4.size(),10);
  EXPECT_EQ(v4[5],1);
  EXPECT_EQ(v4[9],1);
  v4 = v;
  EXPECT_EQ(v4.size(),3);
  EXPECT_EQ(v4[2],2);
  EXPECT_EQ(v4[1],1);
  EXPECT_EQ(v4[0],0);
  VectorInt v5(10, 2);
  EXPECT_EQ(v5.size(),10);
  EXPECT_EQ(v5[7],2);
  EXPECT_EQ(v5[0],2);
  EXPECT_EQ(v.size(),3);
  v = v5;
  EXPECT_EQ(v.size(),10);
  EXPECT_EQ(v[2],2);
  EXPECT_EQ(v[9],2);
  VectorInt cc;
  for (int i = 0; i < 33; ++i) {
    cc.push_back(i);
    EXPECT_EQ(cc.size(),i+1);
    EXPECT_EQ(cc[i],i);
    for (int j=0;j<i;++j)
      EXPECT_EQ(cc[j],j);
  }
  for (int i = 0; i < 33; ++i)
    EXPECT_EQ(cc[i],i);
  cc.resize(20);
  EXPECT_EQ(cc.size(),20);
  for (int i = 0; i < 20; ++i)
    EXPECT_EQ(cc[i],i);
  cc[0]=-1;
  cc.resize(1, 999);
  EXPECT_EQ(cc.size(),1);
  EXPECT_EQ(cc[0],-1);
  cc.resize(99, 99);
  for (int i = 1; i < 99; ++i) {
    //cerr << i << " " << cc[i] << endl;
    EXPECT_EQ(cc[i],99);
  }
  cc.clear();
  EXPECT_EQ(cc.size(),0);
}

template <class VectorInt>
void test_small_vector_2()
{
  using namespace std;
  VectorInt v;
  VectorInt v1(1,0);
  VectorInt v2(2,10);
  VectorInt v1a(2,0);
  EXPECT_TRUE(v1 != v1a);
  EXPECT_EQ_NOPRINT(v1,v1);
  EXPECT_EQ(v1[0], 0);
  EXPECT_EQ(v2[1], 10);
  EXPECT_EQ(v2[0], 10);
  ++v2[1];
  --v2[0];
  EXPECT_EQ(v2[0], 9);
  EXPECT_EQ(v2[1], 11);

  //test shrinking/growing near max inline size (2)
  v2.push_back(12);
  EXPECT_EQ(v2[0], 9);
  EXPECT_EQ(v2[1], 11);
  EXPECT_EQ(v2.size(),3u);
  EXPECT_EQ(v2[2], 12);
  v2.pop_back();
  EXPECT_EQ(v2.size(),2u);
  EXPECT_EQ(v2[0], 9);
  EXPECT_EQ(v2[1], 11);
  v2.pop_back();
  EXPECT_EQ(v2.size(),1u);
  EXPECT_EQ(v2[0], 9);
  v2.push_back(11);
  EXPECT_EQ(v2.size(),2u);
  EXPECT_EQ(v2[0], 9);
  EXPECT_EQ(v2[1], 11);

  VectorInt v3(v2);
  EXPECT_EQ(v3[0],9);
  EXPECT_EQ(v3[1],11);
  assert(!v3.empty());
  EXPECT_EQ(v3.size(),2);
  v3.clear();
  assert(v3.empty());
  EXPECT_EQ(v3.size(),0);
  EXPECT_TRUE(v3 != v2);
  EXPECT_TRUE(v2 != v3);
  v3 = v2;
  EXPECT_EQ_NOPRINT(v3,v2);
  EXPECT_EQ_NOPRINT(v2,v3);
  EXPECT_EQ(v3[0],9);
  EXPECT_EQ(v3[1],11);
  assert(!v3.empty());
  EXPECT_EQ(v3.size(),2);
  VectorInt v4=v3;
  VectorInt v3b=v3;
  v3.append(v4.begin()+1,v4.end());
  EXPECT_EQ(v3.size(),3);
  EXPECT_EQ(v3[2],11);
  v3b.insert(v3b.end(),v4.begin()+1,v4.end());
  EXPECT_EQ(v3b.size(),3);
  EXPECT_EQ(v3b[2],11);

  v4=v3; // avoid aliasing
  v3.append(v4.begin(),v4.end());
  v4=v3;
  EXPECT_EQ(v3.size(),6);
  v3.append((int const *)v4.begin(),(int const *)v4.end());
  EXPECT_EQ(v3.size(),12);
  EXPECT_EQ(v3[11],11);
  VectorInt v5(10);
  EXPECT_EQ(v5[1],0);
  EXPECT_EQ(v5.size(),10);
  v3.assign(v4.begin(),v4.end());
  EXPECT_EQ_NOPRINT(v3,v4);
  int i=2;

  v4.assign(&i,&i);
  EXPECT_EQ(v4.size(),0);
  v4.assign(v1.begin(),v1.end());
  EXPECT_EQ_NOPRINT(v4,v1);
  v4.assign(&i,&i+1);
  EXPECT_EQ(v4.size(),1);
  EXPECT_EQ(v4[0],i);

  v4.insert(v4.begin(),1);
  EXPECT_EQ(v4.size(),2);
  EXPECT_EQ(v4[0],1);
  EXPECT_EQ(v4[1],i);

  VectorInt v4c=v4;
  v4.insert(v4.begin(),v4c.begin(),v4c.end());
  EXPECT_EQ(v4.size(),4);
  EXPECT_EQ(v4[0],1);
  EXPECT_EQ(v4[2],1);
  EXPECT_EQ(v4[1],i);
  EXPECT_EQ(v4[3],i);

  v4.push_front(4);
  EXPECT_EQ(v4.size(),5);
  EXPECT_EQ(v4[0],4);
  EXPECT_EQ(v4[3],1);
  EXPECT_EQ(v4[2],i);
  EXPECT_EQ(v4[4],i);

  VectorInt vg=v4;
  EXPECT_TRUE(vg<=v4);
  EXPECT_TRUE(vg>=v4);
  EXPECT_TRUE(!(vg<v4));
  EXPECT_TRUE(!(v4<vg));

  vg[2]=i+1;
  EXPECT_TRUE(vg>=v4);
  EXPECT_TRUE(vg>v4);
  EXPECT_TRUE(v4<vg);
  vg[2]=v4[2]-1;
  vg[1]=v4[1]+1;
  EXPECT_TRUE(vg>=v4);
  EXPECT_TRUE(vg>v4);
  EXPECT_TRUE(v4<vg);
  vg[0]=v4[1]-1;
  EXPECT_TRUE(v4>=vg);
  EXPECT_TRUE(v4>vg);
  EXPECT_TRUE(vg<v4);
}

BOOST_AUTO_TEST_CASE( test_small_vector_larger_than_2 ) {
  test_small_vector_1<small_vector<int,1> >();
  test_small_vector_1<small_vector<int,3,std::size_t> >();
  test_small_vector_1<small_vector<int,5,unsigned short> >();
}

BOOST_AUTO_TEST_CASE( test_small_vector_small ) {
  test_small_vector_2<small_vector<int,1> >();
  test_small_vector_2<small_vector<int,3,std::size_t> >();
  test_small_vector_2<small_vector<int,5,unsigned short> >();
}

BOOST_AUTO_TEST_CASE(small_vector_compatible_serialization) {
  test_small_vector_same_serializations<small_vector<int,1> >();
  test_small_vector_same_serializations<small_vector<int,3,std::size_t> >();
  test_small_vector_same_serializations<small_vector<int,5,unsigned short> >();
}

}}

#endif
//GRAEHL_TEST

#endif
