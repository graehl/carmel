#ifndef GRAEHL__SHARED__aligned_dynarray_hpp
#define GRAEHL__SHARED__aligned_dynarray_hpp

//TOOD: delete this file. nobody uses it.

#define DBG_ALIGNED_DYN(x) x
// like std::vector but exposes contiguous-array-implementation  - only for types where you can use memcpy to move/swap (thus, also more efficient).
// historical justification: when Carmel was first written, STL wasn't supported by gcc.

#include <graehl/shared/show.hpp>
#include <graehl/shared/dynamic_array.hpp>
#include <stdint.h>
#include <algorithm>
#include <graehl/shared/umod.hpp>

namespace graehl {

//TODO: this is copy/paste from dynamic_array.hpp.  CRTP implementation in terms of padding, or make dynamic_array = aligned_dynamic_array (oblivious to default template args)

// the doubling of active storage amount may be bad given padding in terms of heap frag.  change to padding-aware increased size request?

// caveat:  cannot hold arbitrary types T with self or mutual-pointer refs; only works when memcpy can move you
// asks for an extra 2*byte_multiple bytes when allocating, so that we can place vec[aligned_offset] on an exact multiple of byte_multiple.  that is, the raw space will have some padding because we can't predict the alignment of allocated storage
template <class T,class Alloc=std::allocator<T>,unsigned aligned_index=0,unsigned byte_multiple=64>
class aligned_dynamic_array : public array<T,Alloc> {
/*
  From Pentium IV, System Programming Guide, Section 9.1, Page 9-2,
Table 9-1. Order # 245472

L1 Data Cache - Pentium 4 and Intel Xeon processors: 8 KBytes, 4-way set
associative, 64-byte cache line size.

L2 Unified Cache - Pentium 4 and Intel Xeon processors: 256 KBytes 8-way set
associative, sectored, 64-byte cache line size.

The point is that according to the specs both L1 and L2 cacheline sizes are
64-byte.
*/
public:
  typedef array<T,Alloc> Base;
  typedef aligned_dynamic_array<T,Alloc,aligned_index,byte_multiple> self_type;
  typedef typename Base::iterator iterator;
  typedef typename Base::const_iterator const_iterator;
  typedef typename Base::const_reverse_iterator const_reverse_iterator;
  typedef typename Base::reverse_iterator reverse_iterator;
  typedef typename Base::size_type size_type;
  static inline unsigned byte_offset_0(void *a) { // a is whatever we got from allocator
    /* The binary % operator yields the remainder from the division of the first expression by the second. .... If both operands are nonnegative then the remainder is nonnegative; if not, the sign of the remainder is implementation-defined - seriously, wtf. */
    return unegmod((uintptr_t)a+(sizeof(T)*aligned_index),byte_multiple);
  }
  static const unsigned extra_alloc_bytes=byte_multiple-1;
  static const unsigned extra_alloc_i=(extra_alloc_bytes+sizeof(T)-1)/sizeof(T); // ceil (extra_alloc_bytes/sizeof(T))
private:
  T *begin_alloc;
  unsigned alloc_sz;
  T *endv;
  T *allocate(T *& begin_alloc,unsigned sp) {
    alloc_sz=sp+extra_alloc_i;
    begin_alloc=Base::allocate(alloc_sz);
    size_type offset=byte_offset_0(begin_alloc);
    SHOW7(DBG_ALIGNED_DYN,alloc_sz,begin_alloc,(uintptr_t)begin_alloc % byte_multiple,offset,offset/sizeof(T),((uintptr_t)begin_alloc+offset+aligned_index*sizeof(T))%byte_multiple,byte_multiple);
    return (T*)((char *)begin_alloc+offset);
  }
  void alloc(unsigned sp) {
    if (sp) {
      this->vec=allocate(begin_alloc,sp);
      this->endspace=this->vec+sp;
    } else {
      this->vec=0; // this is what you check when you go to dealloc
    }
  }

private:
  aligned_dynamic_array& operator = (const aligned_dynamic_array &a){std::cerr << "unauthorized assignment of a dynamic array\n";dynarray_assert(0);}
public:

  void swap(self_type &b) throw()
  {
    Base::swap(b);
    std::swap(endv,b.endv);
    std::swap(begin_alloc,b.begin_alloc);
    std::swap(alloc_sz,b.alloc_sz);
  }

  inline friend void swap(self_type &a,self_type &b) throw()
  {
    a.swap(b);
  }

  explicit aligned_dynamic_array (const char *c) {
    std::istringstream(c) >> *this;
  }

  template <class I>
  aligned_dynamic_array(I source,unsigned n,unsigned sp) {
    assert(sp>=n);
    alloc(sp);
    endv=this->vec+n;
    uninitialized_copy_n(source,n,this->vec);
  }

  static inline void uninitialized_copy_n(T const* from,unsigned n,T *to) {
    std::memcpy(to,from,n*sizeof(T));
  }

  template <class I>
  aligned_dynamic_array(T const* p,unsigned n,unsigned sp) {
    assert(sp>=n);
    alloc(sp);
    endv=this->vec+n;
    uninitialized_copy_n(p,n,this->vec);
  }

  // creates vector with CAPACITY for sp elements; size()==0; doesn't initialize (still use push_back etc)
  explicit aligned_dynamic_array(size_type sp = 4) {
    alloc(sp);
    endv=this->vec;
  }

  // creates vector holding sp copies of t; does initialize
  template <class V>
  explicit aligned_dynamic_array(size_type sp,const V& t) {
    alloc(sp);
    endv=this->vec+sp;
    construct(t);
    dynarray_assert(invariant());
  }

  void construct() {
    for (T *p=this->vec;p<endv;++p)
      new(p) T();
    dynarray_assert(p==endv);
  }
  void construct(const T& t) {
    for (T *p=this->vec;p<endv;++p)
      new(p) T(t);
    dynarray_assert(p==endv);
  }

  void reinit(size_type sp,const T& t=T()) {
    clear();
    reinit_nodestroy(sp,t);
  }
  void reinit_nodestroy(size_type sp,const T& t=T()) {
    reserve(sp);
    endv=this->vec+sp;
    construct(t);
  }

  aligned_dynamic_array(const self_type &a) {
//      size_type sz=a.size();
    //alloc(sz);
    alloc(a.size());
    endv=std::uninitialized_copy(a.begin(),a.end(),this->vec);
    dynarray_assert(this->invariant());
  }

  template<class RIT>
  aligned_dynamic_array(typename boost::disable_if<boost::is_arithmetic<RIT>,RIT>::type const& a,RIT const& b)
  {
    alloc(b-a);
    endv=std::uninitialized_copy(a,b,this->begin());
    dynarray_assert(this->invariant());
  }

  template<class Iter>
  void append_n(Iter a,unsigned n) {
    reserve(size()+n);
    endv=uninitialized_copy_n(a,n,endv);
  }

  template<class Iter>
  void append_ra(Iter a,Iter const& b) {
    append_n(a,b-a);
  }

  template <class C>
  void append(C const& c) {
    unsigned n=c.size();
    append_n(c.begin(),n);
  }

  template<class Iter>
  void append(Iter a,Iter const& b)
  {
    for (;a!=b;++a) {
      push_back(*a);
    }
  }

  template<class Iter>
  void set(Iter a,Iter const& b)
  {
    clear();
    append(a,b);
  }

  template <class C>
  void set(C const& c)
  {
    clear();
    reserve(c.size());
    append(c.begin(),c.end());
  }

  // warning: stuff will still be destructed!
  void copyto(T *to,T * from,size_type n) {
    uninitialized_copy_n(from,n,to);
//    memcpy(to,from,bytes(n));
  }
  void copyto(T *to,size_type n) {
    copyto(to,this->vec,n);
  }
  void copyto(T *to) {
    copyto(to,size());
  }

  void moveto(T *to) {
    copyto(to);
    clear_nodestroy();
  }

  const T* end() const { // array code that uses vec+space for boundschecks is duplicated below
    dynarray_assert(this->invariant());
    return endv;
  }


  const_reverse_iterator rbegin() const
  {
    return const_reverse_iterator(end());
  }

  reverse_iterator rbegin()
  {
    return reverse_iterator(end());
  }

  T* end()  { // array code that uses vec+space for boundschecks is duplicated below
    dynarray_assert(this->invariant());
    return endv;
  }
  const T* const_end() const {
    return endv;
  }
  const T* cend() const {
    return endv;
  }

  // move a chunk [i,end()) off the back, leaving the vector as [vec,i)
  void move_rest_to(T *to,iterator i) {
    dynarray_assert(i >= this->begin() && i < end());
    copyto(to,i,this->end()-i);
    endv=i;
  }

  T & at(size_type index) const { // run-time bounds-checked
    T *r=this->vec+index;
    if (!(r < end()) )
      throw std::out_of_range("aligned_dynamic_array index out of bounds");
    return *r;
  }
  bool exists(size_type index) const
  {
    return this->begin()+index < end();
  }

  T & operator[] (size_type index) const {
    dynarray_assert(this->invariant());
    dynarray_assert(this->vec+index < end());
    return (this->vec)[index];
  }
  size_type index_of(T *t) const {
    dynarray_assert(t>=this->begin() && t<end());
    return (size_type)(t-this->vec);
  }

  // NEW OPERATIONS:
  // like [], but bounds-safe: if past end, expands and default constructs elements between old max element and including new max index
  T & operator() (size_type index) {
    if ( index >= size() ) {
      size_type newSpace = this->capacity();
      if (index >= newSpace) {
        if (newSpace==0)
          newSpace = index+1;
        else
          do { newSpace *=2; } while ( index >= newSpace ) ; //FIXME: overflow if newSpace > 2^31
        realloc_up(newSpace);
      }
      T *v = end();
      endv=this->vec+index+1;
      while( v < endv )
        new(v++) T();
    }
    return (this->vec)[index];
  }
  void push(const T &it) {
    push_back(it);
  }
  T &top() {
    return back();
  }
  void pop_back() {
    (--endv)->~T();;
  }

  void pop() {
    pop_back();
  }

  // default-construct version (not in STL vector)
  void push_back()
  {
    new(push_back_raw()) T();
  }
  template <class T0>
  inline void push_back(T0 const& t0)
  {
    new(push_back_raw()) T(t0);
  }
  template <class T0,class T1>
  inline void push_back(T0 const& t0,T1 const& t1)
  {
    new(push_back_raw()) T(t0,t1);
  }
  template <class T0,class T1,class T2>
  inline void push_back(T0 const& t0,T1 const& t1,T2 const& t2)
  {
    new(push_back_raw()) T(t0,t1,t2);
  }

  void push_back_n(const T& val,size_type n)
  {
    dynarray_assert(invariant());
    T *newend=endv+n;
    if (newend > this->endspace) {
      reserve_at_least(size()+n);
      newend=endv+n;
    }

    for (T *p=endv;p!=newend;++p)
      new(p) T(val);
    endv=newend;
    dynarray_assert(invariant());
  }

  // non-construct version (use placement new yourself) (not in STL vector either)
  T *push_back_raw()
  {
    if ( endv >= this->endspace ) {
      if (this->vec == this->endspace )
        realloc_up(4);
      else
        realloc_up(this->capacity()*2); // FIXME: 2^31 problem
    }
    return endv++;
  }
  void undo_push_back_raw() {
    --endv;
  }
  bool empty() const {
    return size()==0;
    //    return vec==endvec;
  }


  template <class Better_than_pred>
  void push_keeping_front_best(T &t,Better_than_pred better)
  {
    throw "untested";
    push_back(t);
    if (empty() || better(front(),t))
      return;
    swap(front(),back());
  }

  T &at_grow(size_type index) {
    T *r=this->vec+index;
    if (r >= end()) {
      if (r >= this->endspace) {
        reserve_at_least(index+1); // doubles if it resizes at all
        r=this->vec+index;
      }
      T *i=end();
      for (;i<=r;++i)
        new(i) T();
      endv=i;
    }
    return *r;
  }
  const T& front()  const {
//        dynarray_assert(size());
    return *this->begin();
  }
  const T& back() const {
//        dynarray_assert(size());
    return *(end()-1);
  }

  T& front() {
    dynarray_assert(size());
    return *this->begin();
  }
  T& back() {
    dynarray_assert(size());
    return *(end()-1);
  }

  void removeMarked(bool marked[]) {
    graehl::remove_marked_swap(*this,marked);
  }

  void removeMarked_nodestroy(bool marked[]) {
    size_type sz=size();
    if ( !sz ) return;
    size_type f, i = 0;
    while ( i < sz && !marked[i] ) ++i;
    f = i; // find first marked (don't need to move anything below it)
    if (i<sz) {
      (this->vec)[i++].~T();
      for(;;) {
        while(i<sz && marked[i])
          (this->vec)[i++].~T();
        if (i==sz)
          break;
        size_type i_base=i;
        while (i<sz && !marked[i]) ++i;
        if (i_base!=i) {
          size_type run=(i-i_base);
//                      DBP(f << i_base << run);
          memmove(&(this->vec)[f],&(this->vec)[i_base],bytes(run));
          f+=run;
        }
      }
    }
    set_size(f);
  }
  bool invariant() const {
    return endv >= this->vec && endv <= this->endspace;
    // && this->endspace > this->vec; //(compact of 0-size dynarray -> 0 capacity!)
  }
//  operator T *() { return this->vec; } // use at own risk (will not be valid after resize)
  // use begin() instead
protected:
  void construct_n_more(size_type n)
  {
    Assert(endv+n <= this->endspace);
    while (n--)
      new (endv++) T();
  }

  void resize_up(size_type new_sz)
  {
    size_type sz=size();
    dynarray_assert(new_sz > size());
    realloc_up(new_sz);
    construct_n_more(new_sz-sz);
    Assert(size()==new_sz);
  }

  void realloc_up(size_type new_cap) {
    size_type sz=size();
    dynarray_assert(new_cap > this->capacity());
    self_type bigger((T const*)this->vec,sz,new_cap);
    bigger.swap(*this);
  }
  void dealloc_safe() {
    if (this->vec) {
      Base::deallocate(begin_alloc,alloc_sz);
      this->vec=0;
    }
  }
public:

#if GRAEHL_DYNARRAY_POD_ONLY_SERIALIZE
  // note: using this implies a trivial destructor / POD copy constructor (which resizable dynarray already exploits)
  template <class A>
  void serialize(A &a) {
    size_type sz;
    if (A::is_saving) sz=size();
    a&sz;
    if (A::is_loading) {
      reserve_at_least(sz);
      set_size(sz);
    }
    a.binary(this->vec,bytes(sz));
  }
#else
  template <class A>
  void serialize(A &a)
  {
    serialize_container(a,*this);
  }

#endif
  // doesn't compact (allocate smaller space).  call compact().  does call destructors
  void resize(size_type newSz) {
    dynarray_assert(invariant());
    //    if (newSz==0) newSz=1;
    size_type sz=size();
    if (newSz<sz) {
      reduce_size(newSz);
      return;
    }
    if (sz==newSz)
      return;
    resize_up(newSz);
    assert(size()==newSz);
  }
  //iterator_tags<Input> == random_access_iterator_tag
  template <class Input>
  void insert(iterator pos,Input from,Input to)
  {
    if (pos==end()) {
      append(from,to);
    } else {
      size_type n_new=from-to;
      reserve(size()+n_new);
      //FIXME: untested
      assert(!"untested");
      T *pfrom=endv;
      endv+=n_new;
      T *pto=endv;
      while (pfrom!=pos)
        *--pto=*--pfrom;
      // now [pfrom,endv) have been shifted right by n_new
      while(pto!=pos) { // this should now happen n_new times
        dynarray_assert(to!=from);
        *--pto=*--to;
      }
    }
  }

  // could just use copy, back_inserter
  template <class Input>
  void append_push_back(Input from,Input to)
  {
    while (from!=to)
      push_back(*from++);
  }

  static inline size_type bytes(size_type n)
  {
    return sizeof(T)*n;
  }

  void compact() {
    size_type sz=size();
    self_type bigger((T const*)this->vec,sz,sz);
    bigger.swap(*this);
    dynarray_assert(invariant());
  }

  // doesn't dealloc *into
  void compact(array<T,Alloc> &into) {
    size_type sz=size();
    into.alloc(sz);
    copyto(into.begin());
  }

  // doesn't dealloc *into
  void compact_giving(array<T,Alloc> &into) {
    size_type sz=size();
    into.alloc(sz);
    copyto(into.begin());
    clear_nodestroy();
  }

  void reserve(size_type newSpace) {
    if (newSpace > this->capacity())
      realloc_up(newSpace);
  }

  void reserve_at_least(size_type req) {
    size_type newcap=this->capacity();
    if (req > newcap) {
      if (newcap==0)
        realloc_up(req);
      else {
        do {
          newcap *=2;
        } while (req > newcap); //FIXME: could loop forever if you have >= 2^31 capacity already
        realloc_up(newcap);
      }
    }
  }
  size_type size() const { return (size_type)(endv-this->vec); }
  void set_size(size_type newSz) { endv=this->vec+newSz; dynarray_assert(this->invariant()); }
  // doesn't compact (free space) unless n==0
  void reduce_size(size_type n) {
    if (n==0) {
      clear_dealloc();
      return;
    }
    T *end=endv;
    reduce_size_nodestroy(n);
    for (T *i=endv;i<end;++i)
      i->~T();
  }
  void reduce_size_nodestroy(size_type n) {
    dynarray_assert(invariant() && n<=size());
    endv=this->vec+n;
  }
  void clear_dealloc_nodestroy()
  {
    dealloc_safe();
    endv=this->vec=this->endspace=0;
  }

  void clear_nodestroy() {
    endv=this->vec;
  }
  void clear() {
    dynarray_assert(invariant());
    for ( T *i=this->begin(),*e=end();i!=e;++i)
      i->~T();
    clear_nodestroy();
  }
  void clear_dealloc() {
    dynarray_assert(invariant());
    for ( T *i=this->begin();i!=end();++i)
      i->~T();
    clear_dealloc_nodestroy();
  }
  ~aligned_dynamic_array() {
    clear();
    dealloc_safe();
  }

  template <class charT, class Traits>
  std::basic_ostream<charT,Traits>& print(std::basic_ostream<charT,Traits>& o,bool multiline=false,bool dummy=false,bool dummy2=false) const
  {
    return range_print(o,this->begin(),end(),DefaultWriter(),multiline);
  }

  template <class charT, class Traits>
  std::basic_ostream<charT,Traits>& print_multiline(std::basic_ostream<charT,Traits>& o) const
  {
    return range_print(o,this->begin(),end(),DefaultWriter(),true);
  }


  template <class charT, class Traits, class Writer >
  std::basic_ostream<charT,Traits>& print(std::basic_ostream<charT,Traits>& o,Writer writer,bool multiline=false) const
  {
    return range_print(o,this->begin(),end(),writer,multiline);
  }

  // if any element read fails, whole array is clobbered (even if appending!)
  // Reader passed by value, so can't be stateful (unless itself is a pointer to shared state)
  template <class charT, class Traits, class Reader>
  std::ios_base::iostate read(std::basic_istream<charT,Traits>& in,Reader read, bool append=false) {
    if (!append)
      clear();

    // slight optimization from not needing temporary like general output iterator version.
    char c;
    EXPECTI_COMMENT_FIRST(in>>c);
    if (c=='(') {
      for(;;) {
        EXPECTI_COMMENT(in>>c);
//                    if (!I_COMMENT(in >> c))                       goto done;
        if (c==')') {
          break;
        }
        in.unget();
        push_back(); // was doing push_back_raw, but that's bad - need to default construct some types where reader assumes constructedness.
        if (!(deref(read)(in,back()))) {
          //undo_push_back_raw();
          goto fail;
        }
//                    if (!I_COMMENT(in >> c))                        goto done;

        EXPECTI_COMMENT(in>>c);
        if (c != ',') in.unget();

      }
    } else {
      in.unget();
      do {
        push_back();
      } while (deref(read)(in,back()));
      if (in.eof()) {
//                    undo_push_back_raw();
        pop_back();
        goto done;
      }
      goto fail;
    }

    //EXPECTCH_SPACE_COMMENT_FIRST('(');
  done:
    dynarray_assert(invariant());
    return GENIOGOOD;
  fail:
    clear();
    return GENIOBAD;
  }
  template <class charT, class Traits>
  std::ios_base::iostate read(std::basic_istream<charT,Traits>& in) {
    return read(in,DefaultReader<T>());
  }
  template <class charT, class Traits>
  std::ios_base::iostate append_from(std::basic_istream<charT,Traits>& in) {
    return read(in,DefaultReader<T>(),array<T,Alloc>::APPEND);
  }
  array<T,Alloc> substr(unsigned start) const
  {
    return substr(start,size());
  }
  TO_OSTREAM_PRINT
  FROM_ISTREAM_READ
  friend inline void push_back(self_type &v) {
    v.push_back();
  }

#undef ALIGNED_ARRAYEQIMP
#define ALIGNED_ARRAYEQIMP                         \
  if (l.size() != r.size()) return false;               \
  LI il=l.begin(),iend=l.end(); \
  RI ir=r.begin();              \
  while (il!=iend)                                      \
    if (!(*il++ == *ir++)) return false;                \
  return true;

template <class L,class A,unsigned a,unsigned b>
friend inline bool operator==(self_type const& l,aligned_dynamic_array<L,A,a,b> const& r)  {
  typedef const_iterator LI;
  typedef typename aligned_dynamic_array<L,A,a,b>::const_iterator RI;
  ALIGNED_ARRAYEQIMP;
}

#define ALIGNED_EQ_OTHER_ARRAY(otype) template <class L,class A> \
friend inline bool operator==(self_type const& l,otype<L,A> const& r) { \
    typedef const_iterator LI;typedef typename otype<L,A>::const_iterator RI;ALIGNED_ARRAYEQIMP    \
} \
template <class L,class A> \
friend inline bool operator==(otype<L,A> const& r,self_type const& l) {        \
    typedef const_iterator LI;typedef typename otype<L,A>::const_iterator RI;ALIGNED_ARRAYEQIMP    \
}

ALIGNED_EQ_OTHER_ARRAY(dynamic_array)
ALIGNED_EQ_OTHER_ARRAY(array)


};


#ifdef GRAEHL_TEST

//FIXME: deallocate everything to make running valgrind on tests less painful
BOOST_AUTO_TEST_CASE( test_aligned_dynarray )
{
  SHOW4(DBG_ALIGNED_DYN,-1,2,umod(-1,2),-1%2 );
  using namespace std;
  {
    const unsigned N=10;

    StackAlloc al;
    unsigned aspace[N];
    al.init(aspace,aspace+N);
    istringstream ina("(1 2 3 4)");
    array<unsigned> aint;
    read(ina,aint,al);
    BOOST_CHECK(aint.size()==4);
    BOOST_CHECK(aint[3]==4);
    BOOST_CHECK(al.top=aspace+4);
  }

  {
    fixed_array<fixed_array<unsigned> > aa,ba;
    std::string sa="(() (1) (1 2 3) () (4))";
    BOOST_CHECK(test_extract_insert(sa,aa));
    IndirectReader<plus_one_reader> reader;
    istringstream ss(sa);

    ba.read(ss,reader);

//        DBP(ba);
    BOOST_REQUIRE(aa.size()==5);
    BOOST_CHECK(aa[2].size()==3);
    BOOST_REQUIRE(ba.size()==5);
    BOOST_CHECK(ba[2].size()==3);
    BOOST_CHECK(aa[1][0]==1);
    BOOST_CHECK(ba[1][0]==2);
  }

  {
    fixed_array<fixed_array<unsigned> > aa,ba;
    std::string sa="(() (1) (1 2 3) () (4))";
    BOOST_CHECK(test_extract_insert(sa,aa));
    IndirectReader<plus_one_reader> reader;
    istringstream ss(sa);

    ba.read(ss,reader);

//        DBP(ba);
    BOOST_REQUIRE(aa.size()==5);
    BOOST_CHECK(aa[2].size()==3);
    BOOST_REQUIRE(ba.size()==5);
    BOOST_CHECK(ba[2].size()==3);
    BOOST_CHECK(aa[1][0]==1);
    BOOST_CHECK(ba[1][0]==2);
  }
  {
    aligned_dynamic_array<unsigned> a;
    a.at_grow(5)=1;
    BOOST_CHECK(a.size()==5+1);
    BOOST_CHECK(a[5]==1);
    for (unsigned i=0; i < 5; ++i)
      BOOST_CHECK(a.at(i)==0);
  }
  const unsigned sz=7;
  {
    aligned_dynamic_array<unsigned> a(sz);
    a.push_back_n(sz,sz*3);
    BOOST_CHECK(a.size() == sz*3);
    BOOST_CHECK(a.capacity() >= sz*3);
    BOOST_CHECK(a[sz]==sz);
  }

  {
    aligned_dynamic_array<unsigned> a(sz*3,sz);
    BOOST_CHECK(a.size() == sz*3);
    BOOST_CHECK(a.capacity() == sz*3);
    BOOST_CHECK(a[sz]==sz);
  }

  using namespace std;
  array<unsigned> aa(sz);
  BOOST_CHECK_EQUAL(aa.capacity(),sz);
  aligned_dynamic_array<unsigned> da;
  aligned_dynamic_array<unsigned> db(sz);
  BOOST_CHECK(db.capacity() == sz);
  copy(a,a+sz,aa.begin());
//  SHOW2(DBG_ALIGNED_DYN,aa,da);
//  copy(a,a+sz,back_inserter(da));
  for (int k=1;k<=7;++k) {
    da.push_back(k);SHOW(DBG_ALIGNED_DYN,da);
  }
  copy(a,a+sz,back_inserter(db));
  SHOW3(DBG_ALIGNED_DYN,aa,da,db);
  BOOST_CHECK(db.capacity() == sz); // shouldn't have grown
  BOOST_CHECK(search(a,a+sz,aa.begin(),aa.end())==a); // really just tests begin,end as proper iterators
  BOOST_CHECK(da.size() == sz);
  BOOST_CHECK(da.capacity() >= sz);
  SHOW2(DBG_ALIGNED_DYN,aa,da);
  BOOST_CHECK(search(a,a+sz,da.begin(),da.end())==a); // tests push_back
  BOOST_CHECK(search(a,a+sz,db.begin(),db.end())==a); // tests push_back
  BOOST_CHECK(search(da.begin(),da.end(),aa.begin(),aa.end())==da.begin());
  for (unsigned i=0;i<sz;++i) {
    BOOST_CHECK(a[i]==aa.at(i));
    BOOST_CHECK(a[i]==da.at(i));
    BOOST_CHECK(a[i]==db(i));
  }
  BOOST_CHECK(da==aa);
  BOOST_CHECK(db==aa);
  const unsigned sz1=3,sz2=4;;
  da.removeMarked_nodestroy(rm1); // removeMarked
  BOOST_REQUIRE(da.size()==sz1);
  for (unsigned i=0;i<sz1;++i)
    BOOST_CHECK(a1[i]==da[i]);
  db.removeMarked_nodestroy(rm2);
  BOOST_REQUIRE(db.size()==sz2);
  for (unsigned i=0;i<sz2;++i)
    BOOST_CHECK(a2[i]==db[i]);
  array<int> d1map(sz),d2map(sz);
  BOOST_CHECK(3==new_indices(rm1,rm1+sz,d1map.begin()));
  BOOST_CHECK(4==new_indices(rm2,rm2+sz,d2map.begin()));
  unsigned c=0;
  for (unsigned i=0;i<d1map.size();++i)
    if (d1map[i]==-1)
      ++c;
    else
      BOOST_CHECK(da[d1map[i]]==aa[i]);

// remove_marked_swap
  std::vector<unsigned> ea(aa.begin(),aa.end());
  aligned_dynamic_array<unsigned> eb(ea.begin(),ea.end());
  BOOST_CHECK_EQUAL_COLLECTIONS(ea.begin(),ea.end(),aa.begin(),aa.end());
  remove_marked_swap(ea,rm1); // removeMarked
  BOOST_REQUIRE(ea.size()==sz1);
  for (unsigned i=0;i<sz1;++i)
    BOOST_CHECK(a1[i]==ea[i]);
  remove_marked_swap(eb,rm2);
  BOOST_REQUIRE(eb.size()==sz2);
  for (unsigned i=0;i<sz2;++i)
    BOOST_CHECK(a2[i]==eb[i]);

  std::vector<int> o2n(aa.size());
  BOOST_CHECK_EQUAL(indices_after_remove_marked(array_begin(o2n),rm1,7),ea.size());
  for (unsigned i=0;i<aa.size();++i)
    if (o2n[i]==-1) {
      BOOST_CHECK_EQUAL(rm1[i],1);
    } else {
      BOOST_CHECK_EQUAL(aa[i],ea[o2n[i]]);
    }


  BOOST_CHECK(c==4);
  db(10)=1;
  BOOST_CHECK(db.size()==11);
  BOOST_CHECK(db.capacity()>=11);
  BOOST_CHECK(db[10]==1);
  aa.dealloc();

  std::string emptya=" ()";
  std::string emptyb="()";
  {
    array<unsigned> a;
    aligned_dynamic_array<unsigned> b;
    istringstream iea(emptya);
    iea >> a;
    stringstream o;
    o << a;
    BOOST_CHECK(o.str()==emptyb);
    o >> b;
    BOOST_CHECK(a==b);
    BOOST_CHECK(a.size()==0);
    BOOST_CHECK(b.size()==0);
  }

  string sa="( 2 ,3 4\n \n\t 5,6)";
  string sb="(2 3 4 5 6)";

#define EQIOTEST(A,B)  do { A<unsigned> a;B<unsigned> b;stringstream o;istringstream isa(sa);isa >> a;  \
    o << a;BOOST_CHECK(o.str() == sb);o >> b;BOOST_CHECK(a==b);} while(0)

  EQIOTEST(array,array);
  EQIOTEST(array,aligned_dynamic_array);
  EQIOTEST(aligned_dynamic_array,array);
  EQIOTEST(aligned_dynamic_array,aligned_dynamic_array);
}
#endif

} //graehl


//FIXME: overriding std::swap is technically illegal, but this allows some dumb std::sort to swap, which it should have found in graehl::swap by arg. dependent lookup.
namespace std {
template <class T,class A>
void swap(graehl::aligned_dynamic_array<T,A> &a,graehl::aligned_dynamic_array<T,A> &b) throw()
{
  a.swap(b);
}
}


#endif
