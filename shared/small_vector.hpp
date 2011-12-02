#ifndef GRAEHL_SHARED___SMALL_VECTOR
#define GRAEHL_SHARED___SMALL_VECTOR

/* REQUIRES that T is POD (can be memcpy).  won't work (yet) due to union with SMALL_VECTOR_POD==0 - may be possible to handle movable types that have ctor/dtor, by using  explicit allocation, ctor/dtor calls.  but for now JUST USE THIS FOR no-meaningful ctor/dtor POD types.

   TODO: need to implement SMALL_VECTOR_POD=0 (will guarantee ctor/dtor calls in that case, incl copy ctor when changing between heap and inline). right now things are assigned, or memcpyd, without ctor/dtor care at all

   stores small element (<=SV_MAX items) vectors inline.  recommend SV_MAX=sizeof(T)/sizeof(T*)>1?sizeof(T)/sizeof(T*):1.  may not work if SV_MAX==0.
 */

#define SMALL_VECTOR_POD 1

#include <algorithm>  // std::max
#include <cstring>
#include <cassert>
#include <boost/cstdint.hpp>
#include <new>
#include <graehl/shared/swap_pod.hpp>
#include <boost/functional/hash.hpp>
#include <graehl/shared/io.hpp>
#include <graehl/shared/genio.h>

//sizeof(T)/sizeof(T*)>1?sizeof(T)/sizeof(T*):1

namespace graehl {

template <class T,int SV_MAX=2,class SIZE=uint16_t>
class small_vector {
//  typedef unsigned short uint16_t;
  void Alloc(size_t s) { // doesn't free old; for ctor. sets size_
    size_=s;
    assert(s <= size_max);
    if (s>SV_MAX) {
      capacity_ = s;
      Alloc_heap();
    }
  }
  void Alloc_heap() {
    data_.ptr=new T[capacity_];  // TODO: replace this with allocator or ::operator new(sizeof(T)*s) everywhere
  }
  void Free_heap() {
    delete[] data_.ptr;
  }
  T *Alloc_heap(size_t s) {
    return new T[s];
  }
  void Free_heap(T *a) {
    delete[] a;
  }

 public:
  typedef small_vector<T,SV_MAX> Self;
  small_vector() : size_(0) {}

  typedef T const* const_iterator;
  typedef T* iterator;
  typedef T value_type;
  typedef T &reference;
  typedef T const& const_reference;
  typedef SIZE size_type;
  static const size_type size_max=
#ifdef NDEBUG
    (size_type)-1;
#else
  ((size_type)-1)/2;
#endif
  T *begin() { return size_>SV_MAX?data_.ptr:data_.vals; }
  T const* begin() const { return const_cast<Self*>(this)->begin(); }
  T *end() { return begin()+size_; }
  T const* end() const { return begin()+size_; }

  explicit small_vector(size_t s) {
    assert(s<=size_max);
    Alloc(s);
    if (s <= SV_MAX) {
      for (size_type i = 0; i < s; ++i) new(&data_.vals[i]) T();
    } //TODO: if alloc were raw space, construct here.
  }

  small_vector(size_t s, T const& v) {
    assert(s<=size_max);
    Alloc(s);
    if (s <= SV_MAX) {
      for (size_type i = 0; i < s; ++i) data_.vals[i] = v;
    } else {
      for (size_type i = 0; i < size_; ++i) data_.ptr[i] = v;
    }
  }

  //TODO: figure out iterator traits to allow this to be selcted for any iterator range.
  template <class I>
  small_vector(I const* begin,I const* end) {
    int s=end-begin;
    assert(s<=size_max);
    Alloc(s);
    if (s <= SV_MAX) {
      for (size_type i = 0; i < s; ++i,++begin) data_.vals[i] = *begin;
    } else
      for (size_type i = 0; i < s; ++i,++begin) data_.ptr[i] = *begin;
  }

  small_vector(T const* i,T const* end) {
    int s=end-i;
    assert(s<=size_max);
    Alloc(s);
    memcpy_from(i);
  }

  small_vector(const Self& o) : size_(o.size_) {
    if (size_ <= SV_MAX) {
      std::memcpy(data_.vals,o.data_.vals,size_*sizeof(T));
//      for (int i = 0; i < size_; ++i) data_.vals[i] = o.data_.vals[i];
    } else {
      capacity_ = size_;
      Alloc_heap();
      memcpy_heap(o.data_.ptr);
    }
  }

  //TODO: test.  this invalidates more iterators than std::vector since resize may move from ptr to vals.
  T *erase(T *b) {
    return erase(b,b+1);
  }
  T *erase(T *b,T* e) { // remove [b,e) and return pointer to element e
    T *tb=begin(),*te=end();
    int nbefore=b-tb;
    if (e==te) {
      resize(nbefore);
    } else {
      int nafter=te-e;
      std::memmove(b,e,nafter*sizeof(T));
      resize(nbefore+nafter);
    }
    return begin()+nbefore;
  }
  void memcpy_to(T *p) {
    std::memcpy(p,begin(),size_*sizeof(T));
  }
  void memcpy_heap(T const* from) {
    assert(size_<=size_max);
    assert(capacity_>=size_);
    std::memcpy(data_.ptr,from,size_*sizeof(T));
  }
  void memcpy_from(T const* from) {
    assert(size_<=size_max);
    std::memcpy(this->begin(),from,size_*sizeof(T));
  }
  static inline void memcpy_n(T *to,T const* from,size_type n) {
    std::memcpy(to,from,n*sizeof(T));
  }

  const Self& operator=(const Self& o) {
    if (size_ <= SV_MAX) {
      if (o.size_ <= SV_MAX) {
        size_ = o.size_;
        for (int i = 0; i < SV_MAX; ++i) data_.vals[i] = o.data_.vals[i];
      } else {
        capacity_ = size_ = o.size_;
        Alloc_heap();
        memcpy_heap(o.data_.ptr);
      }
    } else {
      if (o.size_ <= SV_MAX) {
        Free_heap();
        size_ = o.size_;
        for (int i = 0; i < size_; ++i) data_.vals[i] = o.data_.vals[i];
      } else {
        if (capacity_ < o.size_) {
          Free_heap();
          capacity_ = o.size_;
          Alloc_heap();
        }
        size_ = o.size_;
        memcpy_heap(o.data_.ptr);
        for (int i = 0; i < size_; ++i)
          data_.ptr[i] = o.data_.ptr[i];
      }
    }
    return *this;
  }

  ~small_vector() {
    if (size_ <= SV_MAX) {
      // skip if pod?  yes, we required pod anyway.  no need to destruct
    } else
      Free_heap();
  }

  void clear() {
    Free();
    size_ = 0;
  }
  void set(T const* i,T const* end) {
    Free();
    size_type n=end-i;
    memcpy_n(Realloc(n),i,n);
  }
  template <class I>
  void set_ra(I i,I end) {
    Free();
    T *o=Realloc(end-i);
    for (;i!=end;++i)
      *o++=*i;
  }
  template <class I>
  void set(I i,I end) {
    clear();
    for(;i!=end;++i)
      push_back(*i);
  }


  bool empty() const { return size_ == 0; }
  size_t size() const { return size_; }

  inline void resize_up(size_type s) { // like reserve, but because of capacity_ undef if size_<=SV_MAX, must set size_ immediately or we lose invariant
    assert(s>size_);
    if (s>SV_MAX)
      reserve_up_big(s);
    size_=s;
  }
  void resize_up_big(size_type s) {
    reserve_up_big(s);
    size_=s;
  }
  void reserve(size_type s) {
    if (size_>SV_MAX)
      reserve_up_big(s);
  }

private:
  void Free() {
    if (size_ > SV_MAX)
      Free_heap();
  }
  T *Realloc(size_type s) {
    if (s==size_)
      return begin();
    Free();
    size_=s;
    if (s>SV_MAX) {
      capacity_=size_;
      Alloc_heap();
      return data_.ptr;
    } else
      return data_.vals;
  }

  void Alloc(size_type s) {
    size_=s;
    if (s > SV_MAX) {
      capacity_=s;
      Alloc_heap();
    }
  }


  void reserve_up_big(size_type s) {
    assert(s>SV_MAX);
    if (size_>SV_MAX) {
      if (s>capacity_)
        ensure_capacity(s);
    } else {
      capacity_=s;
      T* tmp = Alloc_heap(s);
      memcpy_n(tmp, data_.vals, SV_MAX); // const instead of size_
      data_.ptr=tmp;
    }
  }
  void ensure_capacity(size_type min_size) {
    // only call if you're already large
    assert(min_size > SV_MAX);
    assert(size_ > SV_MAX);
    if (min_size < capacity_) return;
    size_type new_cap = std::max(static_cast<size_type>(capacity_ << 1), min_size);
    T* tmp = Alloc_heap(new_cap);
    memcpy_n(tmp, data_.ptr, size_);
    Free_heap();
    data_.ptr = tmp;
    capacity_ = new_cap;
  }

  void copy_vals_to_ptr() {
    assert(size_<=SV_MAX);
    capacity_ = SV_MAX * 2;
    T* tmp = Alloc_heap(capacity_);
    memcpy_n(tmp,data_.vals,SV_MAX); // may copy more than actual size_. ok. constant should be more optimizable
    data_.ptr = tmp;
    // only call if you're going to immediately increase size_ to >SV_MAX
  }
  void ptr_to_small() {
    assert(size_<=SV_MAX); // you decreased size_ already. normally ptr wouldn't be used if size_ were this small
    T *tmp=data_.ptr; // should be no problem with memory access order (strict aliasing) because of safe access through union
    for (size_type i=0;i<size_;++i) // no need to memcpy for small size
      data_.vals[i]=tmp[i];
    Free_heap(tmp);
  }

public:

  // aliasing warning: if append is within self, undefined result (because we invalidate iterators as we grow)
  template <class I>
  inline void append(I i,I end) {
    for (;i!=end;++i)
      push_back(*i);
  }

  template <class I>
  inline void append_ra(I i,I e) {
    size_type s=size_;
    resize_up(size_+(e-i));
    assert(size_==s+e-i);
    for (T *b=begin()+s;i<e;++i,++b) {
      assert(b<end());
      *b=*i;
    }
  }
  inline void append(T const* i,T const* end) {
    size_type s=size_;
    size_type n=end-i;
    resize_up(size_+n);
    memcpy_n(begin()+s,i,n);
  }

  inline void push_back(T const& v) {
    if (size_ < SV_MAX) {
      data_.vals[size_] = v;
      ++size_;
    } else if (size_ == SV_MAX) {
      copy_vals_to_ptr();
      data_.ptr[SV_MAX]=v;
      size_=SV_MAX+1;
    } else {
      if (size_ == capacity_)
        ensure_capacity(size_+1);
      data_.ptr[size_++] = v;
    }
  }

  T& back() { return this->operator[](size_ - 1); }
  const T& back() const { return this->operator[](size_ - 1); }
  T& front() { return this->operator[](0); }
  const T& front() const { return this->operator[](0); }

  void pop_back() {
    assert(size_>0);
    --size_;
    if (size_==SV_MAX)
      ptr_to_small();
  }

  void compact() {
    compact(size_);
  }

  // size must be <= size_ - TODO: test
  void compact(size_type newsz) {
    assert(newsz<=size_);
    if (size_>SV_MAX) { // was heap
      size_=newsz;
      if (newsz<=SV_MAX) // now small
        ptr_to_small();
    } else // was small already
      size_=newsz;
  }

  void resize(size_t s, int v = 0) {
    assert(s<=size_max);
    assert(size_<=size_max);
    if (s <= SV_MAX) {
      if (size_ > SV_MAX) {
        size_=s;
        ptr_to_small();
        return;
      } else if (s<=size_) {
      } else { // growing but still small
        for (size_type i = size_; i < s; ++i)
          data_.vals[i] = v;
      }
    } else { // new s is large
      if (s > size_) {
        reserve_up_big(s);
        for (size_type i = size_;i<s;++i)
          data_.ptr[i] = v;
      }
    }
    size_=s;
  }

  T& operator[](size_t i) {
    if (size_ <= SV_MAX) return data_.vals[i];
    return data_.ptr[i];
  }

  const T& operator[](size_t i) const {
    if (size_ <= SV_MAX) return data_.vals[i];
    return data_.ptr[i];
  }

  bool operator==(const Self& o) const {
    if (size_ != o.size_) return false;
    if (size_ <= SV_MAX) {
      for (size_t i = 0; i < size_; ++i)
        if (data_.vals[i] != o.data_.vals[i]) return false;
      return true;
    } else {
      for (size_t i = 0; i < size_; ++i)
        if (data_.ptr[i] != o.data_.ptr[i]) return false;
      return true;
    }
  }

  friend bool operator!=(const Self& a, const Self& b) {
    return !(a==b);
  }

  void swap(Self& o) {
    swap_pod(*this,o);
  }

  inline std::size_t hash_impl() const {
    using namespace boost;
    if (size_==0) return 0;
    if (size_==1) return hash_value(data_.vals[0]);
    if (size_ <= SV_MAX)
      return hash_range(data_.vals,data_.vals+size_);
    return hash_range(data_.ptr,data_.ptr+size_);
  }
//template <class O> friend inline O& operator<<(O &o,small_vector const& x) { return o; }
  typedef void has_print_writer;
  template <class charT, class Traits>
  std::basic_ostream<charT,Traits>& print(std::basic_ostream<charT,Traits>& o,bool multiline=false) const
  {
    return range_print(o,begin(),end(),DefaultWriter(),multiline);
  }
  template <class charT, class Traits, class Writer >
  std::basic_ostream<charT,Traits>& print(std::basic_ostream<charT,Traits>& o,Writer w,bool multiline=false) const
  {
    return range_print(o,begin(),end(),w,multiline);
  }
  typedef small_vector<T,SV_MAX> self_type;
  TO_OSTREAM_PRINT

 private:
  union StorageType { // should be safe from strict-aliasing optimizations
    T vals[SV_MAX]; // iff size_<=SV_MAX (note: tricky! anything that shrinks the array may mean copying back to vals)
    T* ptr; // otherwise
  };
  StorageType data_;
  size_type size_;
  size_type capacity_;  // only initialized when size_ > SV_MAX
};

template <class T,int M>
inline std::size_t hash_value(small_vector<T,M> const& x) {
  return x.hash_impl();
}

template <class T,int M>
inline void swap(small_vector<T,M> &a,small_vector<T,M> &b) {
  a.swap(b);
}

}//ns

#if defined(TEST)
#include <graehl/shared/test.hpp>

typedef graehl::small_vector<int,2> SmallVectorInt;

BOOST_AUTO_TEST_CASE( test_small_vector_larger_than_2 ) {
  using namespace std;
  SmallVectorInt v;
  SmallVectorInt v2;
  v.push_back(0);
  v.push_back(1);
  EXPECT_EQ(v[1],1);
  EXPECT_EQ(v[0],0);
  v.push_back(2);
  EXPECT_EQ(v.size(),3);
  EXPECT_EQ(v[2],2);
  EXPECT_EQ(v[1],1);
  EXPECT_EQ(v[0],0);
  v2 = v;
  SmallVectorInt copy(v);
  EXPECT_EQ(copy.size(),3);
  EXPECT_EQ(copy[0],0);
  EXPECT_EQ(copy[1],1);
  EXPECT_EQ(copy[2],2);
  EXPECT_EQ(copy,v2);
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
  SmallVectorInt v3(1,1);
  EXPECT_EQ(v3[0],1);
  v2 = v3;
  EXPECT_EQ(v2.size(),1);
  EXPECT_EQ(v2[0],1);
  SmallVectorInt v4(10, 1);
  EXPECT_EQ(v4.size(),10);
  EXPECT_EQ(v4[5],1);
  EXPECT_EQ(v4[9],1);
  v4 = v;
  EXPECT_EQ(v4.size(),3);
  EXPECT_EQ(v4[2],2);
  EXPECT_EQ(v4[1],1);
  EXPECT_EQ(v4[0],0);
  SmallVectorInt v5(10, 2);
  EXPECT_EQ(v5.size(),10);
  EXPECT_EQ(v5[7],2);
  EXPECT_EQ(v5[0],2);
  EXPECT_EQ(v.size(),3);
  v = v5;
  EXPECT_EQ(v.size(),10);
  EXPECT_EQ(v[2],2);
  EXPECT_EQ(v[9],2);
  SmallVectorInt cc;
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


BOOST_AUTO_TEST_CASE( test_small_vector_small )
{
  using namespace std;
  SmallVectorInt v;
  SmallVectorInt v1(1,0);
  SmallVectorInt v2(2,10);
  SmallVectorInt v1a(2,0);
  EXPECT_TRUE(v1 != v1a);
  EXPECT_EQ(v1,v1);
  EXPECT_EQ(v1[0], 0);
  EXPECT_EQ(v2[1], 10);
  EXPECT_EQ(v2[0], 10);
  ++v2[1];
  --v2[0];
  EXPECT_EQ(v2[0], 9);
  EXPECT_EQ(v2[1], 11);
  SmallVectorInt v3(v2);
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
  EXPECT_EQ(v3,v2);
  EXPECT_EQ(v2,v3);
  EXPECT_EQ(v3[0],9);
  EXPECT_EQ(v3[1],11);
  assert(!v3.empty());
  EXPECT_EQ(v3.size(),2);
  SmallVectorInt v4=v3;
  v3.append_ra(v4.begin()+1,v4.end());
  EXPECT_EQ(v3.size(),3);
  EXPECT_EQ(v3[2],11);
  v4=v3; // avoid aliasing
  v3.append_ra(v4.begin(),v4.end());
  v4=v3;
  EXPECT_EQ(v3.size(),6);
  v3.append_ra((int const *)v4.begin(),(int const *)v4.end());
  EXPECT_EQ(v3.size(),12);
  EXPECT_EQ(v3[11],11);
  SmallVectorInt v5(10);
  EXPECT_EQ(v5[1],0);
  EXPECT_EQ(v5.size(),10);
  v3.set(v4.begin(),v4.end());
  EXPECT_EQ(v3,v4);
  int i=2;

  v4.set(&i,&i);
  EXPECT_EQ(v4.size(),0);
  v4.set(v1.begin(),v1.end());
  EXPECT_EQ(v4,v1);
  v4.set(&i,&i+1);
  EXPECT_EQ(v4.size(),1);
  EXPECT_EQ(v4[0],i);
//  cerr << "sizeof SmallVectorInt="<<sizeof(SmallVectorInt) << endl;
//  cerr << "sizeof vector="<<sizeof(vector<int>) << endl;
}
#endif

#endif
