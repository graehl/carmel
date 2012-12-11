#ifndef GRAEHL__SHARED__dynamic_array_hpp
#define GRAEHL__SHARED__dynamic_array_hpp

// like std::vector but exposes contiguous-array-implementation  - only for types where you can use memcpy to move/swap (thus, also more efficient). works for nested dynamic_arrays as long as the bottom level is memcpy move/swappable
// historical justification: when Carmel was first written, STL wasn't supported by gcc.

#include <graehl/shared/fixed_array.hpp>

namespace graehl {

// caveat:  cannot hold arbitrary types T with self or mutual-pointer refs; only works when memcpy can move you
// default Alloc defined in fixed_array.hpp
template <typename T,class Alloc> class dynamic_array : public array<T,Alloc> {
public:
  typedef unsigned size_type;
private:
  typedef dynamic_array<T,Alloc> self_type;
  //size_type int sz;
  T *endv;
  typedef array<T,Alloc> Base;
private:
//  dynamic_array& operator = (const dynamic_array &a){std::cerr << "unauthorized assignment of a dynamic array\n";dynarray_assert(0);}
public:
  self_type& operator = (const dynamic_array &a) {
    if (this!=&a) {
      this->~dynamic_array();
      new(this) self_type(a);
    }
    return *this;
  }
  friend std::size_t hash_value(dynamic_array const& x) {
    return boost_hash_range(x.begin(),x.end());
  }
  void swap(self_type &b) throw()
  {
    Base::swap(b);
    std::swap(endv,b.endv);
  }

  inline friend void swap(self_type &a,self_type &b) throw()
  {
    a.swap(b);
  }

  explicit dynamic_array (const char *c) {
    std::istringstream(c) >> *this;
  }

  // creates vector with CAPACITY for sp elements; size()==0; doesn't initialize (still use push_back etc)
  explicit dynamic_array(size_type sp = 4) : array<T,Alloc>(sp), endv(Base::vec) { dynarray_assert(this->invariant()); }

  // creates vector holding sp copies of t; does initialize
  template <class V>
  explicit dynamic_array(size_type sp,const V& t) : array<T,Alloc>(sp) {
    construct(t);
    dynarray_assert(invariant());
  }

  void construct() {
    array<T,Alloc>::construct();
    endv=this->endspace;
  }
  void construct(const T& t) {
    array<T,Alloc>::construct(t);
    endv=this->endspace;
  }

  void reinit(size_type sp,const T& t=T()) {
    clear();
    reinit_nodestroy(sp,t);
  }
  void reinit_nodestroy(size_type sp,const T& t=T()) {
    reserve(sp);
    endv=this->vec+sp;
    for (T *p=this->vec;p!=this->endv;++p)
      new(p) T(t);
  }

  static inline void uninitialized_copy_n_pod(T const* from,unsigned n,T *to) {
    std::memcpy(to,from,n*sizeof(T)); // use only if T is pod.
  }

  static inline void uninitialized_copy_n(T const* from,unsigned n,T *to) {
    for(unsigned i=0;i<n;++i) {
      new(to+i)T(from[i]);
    }
  }

  //FIXME: test:
  template <class RIT>
  dynamic_array(RIT b,RIT e,size_type extra_pre,size_type extra_post) : array<T,Alloc>(e-b+extra_pre+extra_post) {
//    uninitialized_copy_n(a.begin(),sz,this->vec+extra_pre);
    std::uninitialized_copy(b,e,this->vec+extra_pre);
    endv=this->endspace;
    imp_construct_extra(this->vec,endv,extra_pre,extra_post);
    dynarray_assert(this->invariant());
  }
private:
  void imp_init(T const* a,size_type sz,size_type extra_pre,size_type extra_post)  {
    size_type extra_reserve=extra_pre+extra_post;
    this->alloc(sz+extra_reserve);
    uninitialized_copy_n(a,sz,this->vec+extra_pre);
    endv=this->endspace;
    imp_construct_extra(this->vec,endv,extra_pre,extra_post);
    dynarray_assert(this->invariant());
  }
  void imp_construct_extra(T *b,T *e,size_type extra_pre,size_type extra_post) {
    for(T *be=b+extra_pre;b<be;++b)
      new(b)T();
    --e;
    for(T *ee=e-extra_post;e>ee;--e) {
      new(e)T();
    }
  }
public:
  dynamic_array(T const* a,size_type sz,size_type extra_pre,size_type extra_post)  {
    imp_init(a,sz,extra_pre,extra_post);
  }
  dynamic_array(self_type const& a,size_type extra_pre,size_type extra_post)  {
    imp_init(a.begin(),a.size(),extra_pre,extra_post);
  }
  dynamic_array(self_type const& a)  {
    size_type sz=a.size();
    this->alloc(sz);
    endv=this->endspace;
    uninitialized_copy_n(a.begin(),sz,this->vec);
    dynarray_assert(this->invariant());
  }

  template <class RIT>
  dynamic_array(typename boost::disable_if<boost::is_arithmetic<RIT>,RIT>::type const& a,RIT const& b) : array<T,Alloc>(b-a)
  {
    std::uninitialized_copy(a,b,this->vec);
    endv=this->endspace;
    dynarray_assert(this->invariant());
  }

  //FIXME: test:
  template <class Iter>
  void append(Iter a,Iter const& b)
  {
    for (;a!=b;++a) {
      push_back(*a);
    }
  }
  template <class Iter>
  void append_ra(Iter a,Iter const& b)
  {
    reserve(size()+(b-a));
    endv=std::uninitialized_copy(a,b,endv);
  }
  template <class C>
  void append(C const& c)
  {
    reserve(size()+c.size());
    append(c.begin(),c.end());
  }

  template <class Iter>
  void set(Iter a,Iter const& b)
  {
    clear();
    append(a,b);
  }
  template <class Iter>
  void set_ra(Iter a,Iter const& b)
  {
    clear();
    append_ra(a,b);
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
    memcpy(to,from,bytes(n));
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

  typedef typename Base::const_reverse_iterator const_reverse_iterator;
  typedef typename Base::reverse_iterator reverse_iterator;

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
  typedef typename array<T,Alloc>::iterator iterator;
  // move a chunk [i,end()) off the back, leaving the vector as [vec,i)
  void move_rest_to(T *to,iterator i) {
    dynarray_assert(i >= this->vec && i < end());
    copyto(to,i,this->end()-i);
    endv=i;
  }


  T & at(size_type index) const { // run-time bounds-checked
    T *r=this->vec+index;
    if (!(r < end()) )
      throw std::out_of_range("dynamic_array index out of bounds");
    return *r;
  }
  bool exists(size_type index) const
  {
    return this->vec+index < end();
  }

  T & at_assert (size_type index) const {
    assert(this->vec+index < end());
    return (this->vec)[index];
  }
  T & operator[] (size_type index) const {
    dynarray_assert(this->invariant());
    dynarray_assert(this->vec+index < end());
    return (this->vec)[index];
  }
  size_type index_of(T *t) const {
    dynarray_assert(t>=this->vec && t<end());
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
    return *this->vec;
  }
  const T& back() const {
//        dynarray_assert(size());
    return *(end()-1);
  }

  T& front() {
    dynarray_assert(size());
    return *this->vec;
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
    //     we are somehow allowing 0-capacity vectors now?, so add 1
    //if (new_cap==0) new_cap=1;
    size_type sz=size();
    dynarray_assert(new_cap > this->capacity());
    // may be used when we've increased endv past endspace, in order to fix things
    T *newVec = this->allocate(new_cap); // can throw but we've made no changes yet
    memcpy(newVec, this->vec, bytes(sz));
    dealloc_safe();
    this->set_begin(newVec);this->set_capacity(new_cap);set_size(sz);
    // caveat:  cannot hold arbitrary types T with self or mutual-pointer refs
  }
  void dealloc_safe() {
    size_type oldcap=this->capacity();
    if (oldcap)
      this->deallocate(this->vec,oldcap); // can't throw
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
  // doesn't compact (allocate smaller space).  call compact()
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

  //iterator_tags<Input> == random_access_iterator_tag
    /*
  template <class Input>
  void append_ra(Input from,Input to)
  {
      size_type n_new=to-from;
      reserve(size()+n_new);
      while (from!=to)
      new(endv++) T(*from++);
      dynarray_assert(invariant());
    append_n(from,to-from);
  }
    */

  //iterator_tags<Input> == random_access_iterator_tag
  template <class Input>
  void append_n(Input from,size_type n)
  {
    reserve(size()+n);
    while (n-- > 0)
      new(endv++) T(*from++);
//        dynarray_assert(invariant());
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
    dynarray_assert(invariant());
    if (endv==this->endspace) return;
    //equivalent to resize(size());
    size_type newSpace=size();
    //    if (newSpace==0) newSpace=1; // have decided that 0-length dynarray is impossible
    if(newSpace) {
      T *newVec = this->allocate(newSpace); // can throw but we've made no changes yet
      memcpy(newVec, this->vec, bytes(newSpace));
      dealloc_safe();
      //set_begin(newVec);
      //set_capacity(newSpace);set_size(sz);
      this->vec=newVec;
      this->endspace=endv=this->vec+newSpace;
    } else {
      dealloc_safe();
      this->vec=endv=this->endspace=0;
    }
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
    for ( T *i=this->vec,*e=end();i!=e;++i)
      i->~T();
    clear_nodestroy();
  }
  void clear_dealloc() {
    dynarray_assert(invariant());
    for ( T *i=this->vec;i!=end();++i)
      i->~T();
    clear_dealloc_nodestroy();
  }
  ~dynamic_array() {
    clear();
//      if(this->vec) // to allow for exception in constructor
    this->dealloc();
    //vec = NULL;space=0; // don't really need but would be safer
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
};

struct GVectorS {
  template <class T> struct container {
    typedef dynamic_array<T> type;
  };
};

template <typename T,class A>
void push_back(dynamic_array<T,A> &v)
{
  v.push_back();
}


template <typename T,typename Alloc,class charT, class Traits, class Reader>
//std::ios_base::iostate array<T,Alloc>::read(std::basic_istream<charT,Traits>& in,Reader read)
std::ios_base::iostate read_imp(array<T,Alloc> *a,std::basic_istream<charT,Traits>& in,Reader read,unsigned reserve)
{
  dynamic_array<T,Alloc> s(reserve);
  std::ios_base::iostate ret=s.read(in,read);
  a->dealloc();
  s.compact_giving(*a); // transfers to a
  return ret;
}

#if 0
template <class charT, class Traits,class L,class A>
std::basic_istream<charT,Traits>&
operator >>
(std::basic_istream<charT,Traits>& is, dynamic_array<L,A> &arg)
{
  return gen_extractor(is,arg);
}

template <class charT, class Traits,class L,class A>
std::basic_ostream<charT,Traits>&
operator <<
(std::basic_ostream<charT,Traits>& os, const dynamic_array<L,A> &arg)
{
  return gen_inserter(os,arg);
}
#endif

#define ARRAYEQIMP                                      \
  if (l.size() != r.size()) return false;               \
  typename L::const_iterator il=l.begin(),iend=l.end(); \
  typename R::const_iterator ir=r.begin();              \
  while (il!=iend)                                      \
    if (!(*il++ == *ir++)) return false;                \
  return true;


template<class Lt,class A,class L2,class A2>
bool operator ==(const dynamic_array<Lt,A> &l, const dynamic_array<L2,A2> &r)
{
  typedef dynamic_array<Lt,A> L;
  typedef dynamic_array<L2,A2> R;
  ARRAYEQIMP;
}


template<class Lt,class A,class L2,class A2>
bool operator ==(const dynamic_array<Lt,A> &l, const array<L2,A2> &r)
{
  typedef dynamic_array<Lt,A> L;
  typedef array<L2,A2> R;
  ARRAYEQIMP;
}

template<class Lt,class A,class L2,class A2>
bool operator ==(const array<Lt,A> &l, const dynamic_array<L2,A2> &r)
{
  typedef array<Lt,A> L;
  typedef dynamic_array<L2,A2> R;
  ARRAYEQIMP;
}


template<class Lt,class A,class L2,class A2>
bool operator ==(const array<Lt,A> &l, const array<L2,A2> &r)
{
  typedef array<Lt,A> L;
  typedef array<L2,A2> R;
  ARRAYEQIMP;
}


template <class L,class A>
void read(std::istream &in,array<L,A> &x,StackAlloc &a)
// throw(genio_exception,StackAlloc::Overflow)
{
  x.vec=a.aligned_next<L>();
  function_output_iterator<boost::reference_wrapper<StackAlloc> > out(boost::ref(a));
  range_read(in,out,DefaultReader<L>());
  x.endspace=a.next<L>();
}


#ifdef GRAEHL_TEST

//FIXME: deallocate everything to make running valgrind on tests less painful

bool rm1[] = { 0,1,1,0,0,1,1 };
bool rm2[] = { 1,1,0,0,1,0,0 };
unsigned a[] = { 1,2,3,4,5,6,7 };
unsigned a1[] = { 1, 4, 5 };
unsigned a2[] = {3,4,6,7};
#include <algorithm>
#include <iterator>
struct plus_one_reader {
  typedef unsigned value_type;
  template <class charT, class Traits>
  std::basic_istream<charT,Traits>&
  operator()(std::basic_istream<charT,Traits>& in,unsigned &l) const {
    in >> l;
    ++l;
    return in;
  }
};
BOOST_AUTO_TEST_CASE( test_dynarray )
{
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
    dynamic_array<unsigned> a;
    a.at_grow(5)=1;
    BOOST_CHECK(a.size()==5+1);
    BOOST_CHECK(a[5]==1);
    for (unsigned i=0; i < 5; ++i)
      BOOST_CHECK(a.at(i)==0);
  }
  const unsigned sz=7;
  {
    dynamic_array<unsigned> a(sz);
    a.push_back_n(sz,sz*3);
    BOOST_CHECK(a.size() == sz*3);
    BOOST_CHECK(a.capacity() >= sz*3);
    BOOST_CHECK(a[sz]==sz);
  }

  {
    dynamic_array<unsigned> a(sz*3,sz);
    BOOST_CHECK(a.size() == sz*3);
    BOOST_CHECK(a.capacity() == sz*3);
    BOOST_CHECK(a[sz]==sz);
  }

  using namespace std;
  array<unsigned> aa(sz);
  BOOST_CHECK_EQUAL(aa.capacity(),sz);
  dynamic_array<unsigned> da;
  dynamic_array<unsigned> db(sz);
  BOOST_CHECK(db.capacity() == sz);
  copy(a,a+sz,aa.begin());
  copy(a,a+sz,back_inserter(da));
  copy(a,a+sz,back_inserter(db));
  BOOST_CHECK(db.capacity() == sz); // shouldn't have grown
  BOOST_CHECK(search(a,a+sz,aa.begin(),aa.end())==a); // really just tests begin,end as proper iterators
  BOOST_CHECK(da.size() == sz);
  BOOST_CHECK(da.capacity() >= sz);
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
  unsigned c=0;

  {
  array<int> d1map(sz),d2map(sz);
  BOOST_CHECK(3==new_indices(rm1,rm1+sz,d1map.begin()));
  BOOST_CHECK(4==new_indices(rm2,rm2+sz,d2map.begin()));
  for (unsigned i=0;i<d1map.size();++i)
    if (d1map[i]==-1)
      ++c;
    else
      BOOST_CHECK(da[d1map[i]]==aa[i]);
  d1map.dealloc();d2map.dealloc();
  }

// remove_marked_swap
  std::vector<unsigned> ea(aa.begin(),aa.end());
  dynamic_array<unsigned> eb(ea.begin(),ea.end());
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
    dynamic_array<unsigned> b;
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
    o << a;BOOST_CHECK(o.str() == sb);o >> b;BOOST_CHECK(a==b);a.dealloc();b.dealloc();} while(0)

  EQIOTEST(array,array);
  EQIOTEST(array,dynamic_array);
  EQIOTEST(dynamic_array,array);
  EQIOTEST(dynamic_array,dynamic_array);
}
#endif

} //graehl


//FIXME: overriding std::swap is technically illegal, but this allows some dumb std::sort to swap, which it should have found in graehl::swap by arg. dependent lookup.
namespace std {
template <class T,class A>
void swap(graehl::dynamic_array<T,A> &a,graehl::dynamic_array<T,A> &b) throw()
{
  a.swap(b);
}
}


#endif
