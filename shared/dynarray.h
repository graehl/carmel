#ifndef DYNARRAY_H
#define DYNARRAY_H 1
// For MEMCPY-able-to-move types only!
// (MEMCPY copyable (i.e. no external resources owned, i.e. no real destructor) is not assumed although a is_pod template might be nice)
// Array encapsulates a region of memory and doesn't own its own storage ... you can create subarrays of arrays that use the same storage.  you could implement vector or string on top of it.  it does take an allocator argument and has methods alloc, dealloc, re_alloc (realloc is a macro in MS, for shame), which need not be used.  as a rule, nothing writing an Array ever deallocs old space automatically, since an Array might not have alloced it.
// DynamicArray extends an array to allow it to be grown efficiently one element at a time (there is more storage than is used) ... like vector but better for MEMCPY-able stuff
// note that DynamicArray may have capacity()==0 (though using push_back and array(i) will allocate it as necessary)

#include "config.h"

#include <string>
#include <new>
#include "myassert.h"
#include <memory>
#include <stdexcept>
#include <iostream>
#include "genio.h"
#include "byref.hpp"
#include <iterator>

#ifdef TEST
#include "../../tt/test.hpp"
#endif
//#include <boost/type_traits.hpp>

namespace nonstd {

// bleh, std::construct also was nonstandard and removed
template <class P,class V>
void construct(P *to,const V& from)
{
  PLACEMENT_NEW(to) P(from);
}

// uninitialized_copy_n was removed from std:: - we don't reuse the name because you might use namespace std; in an old compiler
// plus our version doesn't return the updated iterators
template <class I,class F>
void uninitialized_copy_n(I from, unsigned n, F to)
{
  for ( ; n > 0 ; --n, ++from, ++to)
    //PLACEMENT_NEW(&*to) iterator_traits<F>::value_type(*from);
    construct(&*to,*from);
}
};
// if you want custom actions/parsing while reading labels, make a functor with this signature and pass it as an argument to read_tree (or get_from):
template <class Label>
struct DefaultReader
{
  typedef Label value_type;
  template <class charT, class Traits>
        std::basic_istream<charT,Traits>&
         operator()(std::basic_istream<charT,Traits>& in,Label &l) const {
          return in >> l;
         }
};

struct DefaultWriter
{
  template <class charT, class Traits,class Label>
        std::basic_ostream<charT,Traits>&
         operator()(std::basic_ostream<charT,Traits>& o,const Label &l) const {
          return o << l;
         }
};

// can only be passed to class that itself reads things with a Reader get_from method.
template <class R>
struct IndirectReader
{
    IndirectReader() {}
    R reader;
    IndirectReader(const R& r) : reader(r) {}
    typedef void value_type; // not really
    
    template <class Target,class charT, class Traits>
        std::basic_istream<charT,Traits>&
         operator()(std::basic_istream<charT,Traits>& in,Target &l) const {
        return gen_extractor(in,l,reader);
    }
};

  template <class charT, class Traits, class T,class Writer>
        std::ios_base::iostate range_print_on(std::basic_ostream<charT,Traits>& o,T begin, T end,Writer writer,bool multiline=false)
  {
        o << '(';
        if (multiline) {
#define LONGSEP "\n "
          for (;begin!=end;++begin) {
           o << LONGSEP;
           deref(writer)(o,*begin);
          }
         o << "\n)";

         o << std::endl;
        } else {
          bool first=true;
          for (;begin!=end;++begin) {
                if (first)
                  first = false;
                else
                        o << ' ';
                deref(writer)(o,*begin);
          }
         o << ')';
        }
  return GENIOGOOD;
}

  // modifies out iterator.  if returns GENIOBAD then elements might be left partially extracted.  (clear them yourself if you want)
template <class charT, class Traits, class Reader, class T>
std::ios_base::iostate range_get_from(std::basic_istream<charT,Traits>& in,T &out,Reader read)

{
  char c;

  EXPECTCH_SPACE_COMMENT('(');
  for(;;) {
    EXPECTI_COMMENT(in>>c);
          if (c==')') {
                break;
          }
          in.unget();
#if 1
          typename Reader::value_type temp;
          if (deref(read)(in,temp).good())
                *out++=temp;
          else
                goto fail;
#else
          // doesn't work for back inserter for some reason
          if (!deref(read)(in,*&(*out++)).good()) {
                goto fail;
          }
#endif
          EXPECTI_COMMENT(in>>c);
          if (c != ',') in.unget();
  }
  return GENIOGOOD;
fail:
  return GENIOBAD;
}



// doesn't manage its own space (use alloc(n) and dealloc() yourself).  0 length allowed.
// you must construct and deconstruct elements yourself.  raw dynamic uninitialized (classed) storage array
template <typename T,typename Alloc=std::allocator<T> > class Array : protected Alloc {
    protected:
    //unsigned int space;
    T *vec;
    T *endspace;
    public:
//    operator T*() const { return vec; }
  bool invariant() const {
    return vec >= endspace;
  }
//!FIXME: does this swap allocator base?
void swap(Array<T,Alloc> &a) {
    typedef Array<T,Alloc> Self;
    Self t;
    memcpy(&t,this,sizeof(Self));
    memcpy(this,&a,sizeof(Self));
    memcpy(&a,&t,sizeof(Self));
}

  typedef T value_type;
  typedef value_type *iterator;
  typedef const value_type *const_iterator;
  typedef T &reference;
  typedef const T& const_reference;

  void construct() {
        for (T *p=vec;p!=endspace;++p)
          PLACEMENT_NEW(p) T();
  }
  void construct(const T& val) {
        for (T *p=vec;p!=endspace;++p)
          PLACEMENT_NEW(p) T(val);
  }

        const T& front()  const {
          Assert(size());
          return *begin();
        }
        const T& back() const {
          Assert(size());
          return *(end()-1);
        }

        T& front() {
          Assert(size());
          return *begin();
        }
        T& back() {
          Assert(size());
          return *(end()-1);
        }

  template <class charT, class Traits>
        std::ios_base::iostate print_on(std::basic_ostream<charT,Traits>& o,bool multiline=false) const
  {
        return range_print_on(o,begin(),end(),DefaultWriter(),multiline);
  }

  typedef void has_print_on_writer;
  template <class charT, class Traits, class Writer >
        std::ios_base::iostate print_on(std::basic_ostream<charT,Traits>& o,Writer w,bool multiline=false) const
  {
        return range_print_on(o,begin(),end(),w,multiline);
  }



// Reader passed by value, so can't be stateful (unless itself is a pointer to shared state)
template <class T2,class Alloc2, class charT, class Traits, class Reader> friend
std::ios_base::iostate get_from_imp(Array<T2,Alloc2> *s,std::basic_istream<charT,Traits>& in,Reader read);

template <class charT, class Traits, class Reader>
std::ios_base::iostate get_from(std::basic_istream<charT,Traits>& in,Reader read) {
  dealloc();
  return get_from_imp(this,in,read);
}

  bool empty() const {
        return vec==endspace;
  }
  void dealloc() {
        int cap=capacity();
        if (cap) {
          Assert(vec);
          this->deallocate(vec,cap);
          Paranoid(vec=NULL;);
          endspace=vec;
        }
  }
  void destroy() {
      for ( T *i=begin();i!=end();++i)
                i->~T();
  }


  void copyto(T *to,unsigned n) {
        memcpy(to,vec,sizeof(T)*n);
  }

  void copyto(T *to) {
        copyto(to,capacity());
  }

  // doesn't copy old elements like dynamicarray does
  void alloc(unsigned sp) {
        if(sp)
          vec=this->allocate(sp);
        endspace=vec+sp;
  }
  void re_alloc(unsigned sp) {
//      if (sp == space) return;
          dealloc();
          alloc(sp);
  }
  explicit Array(const char *c) {
        std::istringstream(c) >> *this;
  }
  Array(const T *begin, const T *end) : vec(const_cast<T *>(begin)), endspace(const_cast<T *>(end)) { }
  Array(const T* buf,unsigned sz) : vec(const_cast<T *>(buf)), endspace(buf+sz) {}
  explicit Array(unsigned sp=4) { alloc(sp); }
  Array(unsigned sp, const Alloc &a): Alloc(a) { alloc(sp); }
  template<class I>
  Array(unsigned n,I begin) { // copy up to n
    alloc(n);
    nonstd::uninitialized_copy_n(begin, n, vec);
  }

  unsigned capacity() const { return (unsigned)(endspace-vec); }
  unsigned size() const { return capacity(); }
  T * begin() { return vec; }
        T * end() {       return endspace;      }
        const T* begin() const {
          return vec;
        }
  const T* end() const {
          return endspace;
  }
  T & at(unsigned int index) const { // run-time bounds-checked
        T *r=vec+index;
        if (!(r < end()) )
          throw std::out_of_range("dynarray");
        return *r;
  }
  T & operator[] (unsigned int index) const {
    Assert(vec+index < end());
    return vec[index];
  }
  unsigned int index_of(T *t) const {
        Assert(t>=begin() && t<end());
    return (unsigned int)(t-vec);
  }
  ///XXX: must always properly set_capacity AFTER set_begin since this invalidates capacity
  void set_begin(T* buf) {
        //unsigned cap=capacity();
        vec=buf;
        //set_capacity(cap);
  }
  void set_capacity(unsigned int newCap) { endspace=vec+newCap; }
};

// frees self automatically - WARNING - doesn't copy contents!
template <typename T,typename Alloc=std::allocator<T> > class AutoArray : public Array<T,Alloc> {
public:
  typedef Array<T,Alloc> Super;
  explicit AutoArray(unsigned sp=4) : Super(sp) { }
  ~AutoArray() {
        this->dealloc();
  }
//!FIXME: doesn't swap allocator base
void swap(AutoArray<T,Alloc> &a) {
    Array<T,Alloc>::swap(a);
}
protected:
  AutoArray(AutoArray<T,Alloc> &a) : Super(a.capacity()){
  }
private:
void swap(Array<T,Alloc> &a) {Assert(0);}
};

// FIXME: drop template copy constructor since it screws with one arg size constructor
// frees self automatically; inits/destroys/copies just like DynamicArray but can't be resized.
template <typename T,typename Alloc=std::allocator<T> > class FixedArray : public AutoArray<T,Alloc> {
public:
  typedef AutoArray<T,Alloc> Super;
  explicit FixedArray(unsigned sp=0) : Super(sp) {
    this->construct();
  }
  ~FixedArray() {
    this->destroy();
        //~Super(); // happens implicitly!
  }
template <class charT, class Traits, class Reader>
std::ios_base::iostate get_from(std::basic_istream<charT,Traits>& in,Reader read) {
    this->destroy();
    this->dealloc();
    return get_from_imp(this,in,read);
}

  void uninit_copy_from(const T* b,const T* e) {
    Assert(e-b == this->capacity());
    std::uninitialized_copy(b,e,this->begin());
    //memcpy(begin(),b,e-b);
  }
/*  FixedArray(const Array<T,Alloc> &a) : Super(a.capacity()) {
    DBPC("FixedArray copy",a);
    uninit_copy_from(a.begin(),a.end());
  }
  FixedArray(const AutoArray<T,Alloc> &a) : Super(a.capacity()) {
    DBPC("FixedArray copy",a);
    uninit_copy_from(a.begin(),a.end());
  }
  FixedArray(const DynamicArray<T,Alloc> &a) : Super(a.capacity()) {
    DBPC("FixedArray copy",a);
    uninit_copy_from(a.begin(),a.end());
  }
*/
  //F=FixedArray<T,Alloc>

// having problems with ambiguous constructor?  make sure you pass an *unsigned* int if you want to specify # of elements.  I don't know how to make this constructor exclude integral types, well, I could do it with type traits and a second param is_container<F> *dummy=0
  template <class F>
  FixedArray(const F &a) : Super(a.size()) {
    //    DBPC("FixedArray copy",a);
    uninit_copy_from(a.begin(),a.end());
  }
    
  FixedArray(const FixedArray<T,Alloc>  &a) : Super(a.size()) {
    //    DBPC("FixedArray copy",a);
    uninit_copy_from(a.begin(),a.end());
  }

private:

};


// caveat:  cannot hold arbitrary types T with self or mutual-pointer refs; only works when memcpy can move you
// FIXME: possible for this to not be valid for any object with a default constructor :-(
template <typename T,typename Alloc=std::allocator<T> > class DynamicArray : public Array<T,Alloc> {
  //unsigned int sz;
  T *endv;
  typedef Array<T,Alloc> Base;
  DynamicArray& operator = (const DynamicArray &a){std::cerr << "unauthorized assignment of a dynamic array\n";}; // Yaser
  private:
void swap(Array<T,Alloc> &a) {Assert(0);}
  public:
 public:
  enum { REPLACE=0, APPEND=1 };
  enum { BRIEF=0, MULTILINE=1 };
  explicit DynamicArray (const char *c) {
        std::istringstream(c) >> *this;
  }

  // creates vector with CAPACITY for sp elements; size()==0; doesn't initialize (still use push_back etc)
  explicit DynamicArray(unsigned sp = 4) : Array<T,Alloc>(sp), endv(Base::vec) { Assert(this->invariant()); }

  // creates vector holding sp copies of t; does initialize
  explicit DynamicArray(unsigned sp,const T& t) : Array<T,Alloc>(sp), endv(Base::endspace) {
        construct(t);
        Assert(invariant());
  }

  void construct() {
        Assert(endv=this->vec);
        Array<T,Alloc>::construct();
        endv=this->endspace;
  }
  void construct(const T& t) {
        Assert(endv=this->vec);
        Array<T,Alloc>::construct(t);
        endv=this->endspace;
  }

  DynamicArray(const DynamicArray &a) : Array<T,Alloc>(a.size()) {
//      unsigned sz=a.size();
    //alloc(sz);
//      memcpy(this->vec,a.vec,sizeof(T)*sz);
    std::uninitialized_copy(a.begin(),a.end(),this->begin());
        endv=this->endspace;
    Assert(this->invariant());
  }

  // warning: stuff will still be destructed!
  void copyto(T *to,T * from,unsigned n) {
        memcpy(to,from,sizeof(T)*n);
  }
  void copyto(T *to,unsigned n) {
        copyto(to,this->vec,n);
  }
  void copyto(T *to) {
        copyto(to,size());
  }

  void moveto(T *to) {
        copyto(to);
        clear_nodestroy();
  }

  const T* end() const { // Array code that uses vec+space for boundschecks is duplicated below
    Assert(invariant());
          return endv;
  }
    T* end()  { // Array code that uses vec+space for boundschecks is duplicated below
    Assert(invariant());
          return endv;
  }

  // move a chunk [i,end()) off the back, leaving the vector as [vec,i)
  void move_rest_to(T *to,typename Array<T,Alloc>::iterator i) {
        Assert(i >= this->begin() && i < end());
        copyto(to,i,this->end()-i);
        endv=i;
  }


  T & at(unsigned int index) const { // run-time bounds-checked
        T *r=this->vec+index;
        if (!(r < end()) )
          throw std::out_of_range("dynarray");
        return *r;
  }
  T & operator[] (unsigned int index) const {
    Assert(invariant());
    Assert(this->vec+index < end());
    return (this->vec)[index];
  }
  unsigned int index_of(T *t) const {
        Assert(t>=this->begin() && t<end());
    return (unsigned int)(t-this->vec);
  }

  // NEW OPERATIONS:
  // like [], but bounds-safe: if past end, expands and default constructs elements between old max element and including new max index
  T & operator() (unsigned int index) {
    if ( index >= size() ) {
      unsigned int newSpace = this->capacity();
      if (index >= newSpace) {
        if (newSpace==0)
          newSpace = index+1;
        else
          do { newSpace *=2; } while ( index >= newSpace ) ; //FIXME: overflow if newSpace > 2^31
        resize_up(newSpace);
      }
      T *v = end();
      endv=this->vec+index+1;
      while( v < endv )
        PLACEMENT_NEW(v++) T();
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
      Assert(invariant());
      PLACEMENT_NEW(push_back_raw()) T();
      Assert(invariant());
    }
  void push_back(const T& val)
    {
    Assert(invariant());
      PLACEMENT_NEW(push_back_raw()) T(val);
    Assert(invariant());
    }
  void push_back(const T& val,unsigned n)
  { Assert(invariant());
          T *newend=endv+n;
      if (newend > this->endspace) {
                reserve_at_least(size()+n);
        newend=endv+n;
      }

          for (T *p=endv;p!=newend;++p)
            PLACEMENT_NEW(p) T(val);
      endv=newend;
    Assert(invariant());}

  // non-construct version (use PLACEMENT_NEW yourself) (not in STL vector either)
  T *push_back_raw()
    {
      if ( endv >= this->endspace )
        if (this->vec == this->endspace )
          resize_up(4);
        else
          resize_up(this->capacity()*2); // FIXME: 2^31 problem
      return endv++;
    }
        void undo_push_back_raw() {
          --endv;
        }
  T &at_grow(unsigned index) {
        T *r=this->vec+index;
        if (r >= end()) {
          if (r >= this->endspace) {
                reserve_at_least(index+1); // doubles if it resizes at all
                r=this->vec+index;
          }
          T *i=end();
          for (;i<=r;++i)
                PLACEMENT_NEW(i) T();
          endv=i;
        }
        return *r;
  }
        const T& front()  const {
          Assert(size());
          return *this->begin();
        }
        const T& back() const {
          Assert(size());
          return *(end()-1);
        }

        T& front() {
          Assert(size());
          return *this->begin();
        }
        T& back() {
          Assert(size());
          return *(end()-1);
        }

  void removeMarked(bool marked[]) {
        unsigned sz=size();
    if ( !sz ) return;
    unsigned int f, i = 0;
    while ( i < sz && !marked[i] ) ++i;
          f = i; // find first marked (don't need to move anything below it)
#ifndef OLD_REMOVE_MARKED
          if (i<sz) {
            (this->vec)[i++].~T();
                for(;;) {
                  while(i<sz && marked[i])
                    (this->vec)[i++].~T();
                  if (i==sz)
                        break;
                  unsigned i_base=i;
                  while (i<sz && !marked[i]) ++i;
                  if (i_base!=i) {
                        unsigned run=(i-i_base);
//                      DBP(f << i_base << run);
                        memmove(&(this->vec)[f],&(this->vec)[i_base],sizeof(T)*run);
                        f+=run;
                  }
                }
          }
#else
    while ( i < sz )
      if ( !marked[i] )
                memcpy(&(this->vec)[f++], &(this->vec)[i++], sizeof(T));
      else
                (this->vec)[i++].~T();
#endif
    set_size(f);
  }
  bool invariant() const {
    return endv >= this->vec && endv <= this->endspace;
    // && this->endspace > this->vec; //(compact of 0-size dynarray -> 0 capacity!)
  }
//  operator T *() { return this->vec; } // use at own risk (will not be valid after resize)
  // use begin() instead
  protected:
  void resize_up(unsigned int newSpace) {
    //     we are somehow allowing 0-capacity vectors now?, so add 1
    //if (newSpace==0) newSpace=1;
    Assert(newSpace > this->capacity());
    // may be used when we've increased endv past endspace, in order to fix things
    unsigned sz=size();
    T *newVec = this->allocate(newSpace); // can throw but we've made no changes yet
    memcpy(newVec, this->vec, sz*sizeof(T));
    dealloc_safe();
    set_begin(newVec);this->set_capacity(newSpace);set_size(sz);
    // caveat:  cannot hold arbitrary types T with self or mutual-pointer refs
  }
  void dealloc_safe() {
     unsigned oldcap=this->capacity();
     if (oldcap)
       this->deallocate(this->vec,oldcap); // can't throw
  }
  public:
  void resize(unsigned int newSpace) {
    Assert(invariant());
    //    if (newSpace==0) newSpace=1;
    if (endv==this->endspace) return;
    unsigned sz=size();
    if ( newSpace < sz )
      newSpace = sz;
    if (newSpace) {
      T *newVec = this->allocate(newSpace); // can throw but we've made no changes yet
      memcpy(newVec, this->vec, sz*sizeof(T));
      dealloc_safe();
      set_begin(newVec);this->set_capacity(newSpace);set_size(sz);
    } else {
      dealloc_safe();
      this->vec=endv=this->endspace=0;
    }
    // caveat:  cannot hold arbitrary types T with self or mutual-pointer refs
  }
  void compact() {
    Assert(invariant());
    if (endv==this->endspace) return;
        //equivalent to resize(size());
    unsigned newSpace=size();

    //    if (newSpace==0) newSpace=1; // have decided that 0-length dynarray is impossible
    if(newSpace) {
      T *newVec = this->allocate(newSpace); // can throw but we've made no changes yet
      memcpy(newVec, this->vec, newSpace*sizeof(T));

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
  void compact(Array<T,Alloc> *into) {
        unsigned sz=size();
    into->alloc(sz);
        copyto(into->begin());
  }

  void reserve(unsigned int newSpace) {
        if (newSpace > this->capacity())
          resize(newSpace);
  }
  void reserve_at_least(unsigned req) {
        unsigned newcap=this->capacity();
        if (req > newcap) {
          if (newcap==0)
            resize_up(req);
          else {
            do {
              newcap *=2;
            } while (req > newcap); //FIXME: could loop forever if you have >= 2^31 capacity already
            resize_up(newcap);
          }
        }
  }
  unsigned int size() const { return (unsigned)(endv-this->vec); }
  void set_size(unsigned int newSz) { endv=this->vec+newSz; }
  void reduce_size(unsigned int n) {
    T *end=endv;
    reduce_size_nodestroy(n);
    for (T *i=endv;i<end;++i)
      i->~T();
  }
  void reduce_size_nodestroy(unsigned int n) {
    Assert(invariant() && n<=size());
    endv=this->vec+n;    
  }
  void clear_nodestroy() {
        endv=this->vec;
  }
  void clear() {
    Assert(invariant());
      for ( T *i=this->begin();i!=end();++i)
                i->~T();
    clear_nodestroy();
  }
  ~DynamicArray() {
    clear();
//      if(this->vec) // to allow for exception in constructor
        this->dealloc();
        //vec = NULL;space=0; // don't really need but would be safer
  }


  template <class charT, class Traits>
        std::ios_base::iostate print_on(std::basic_ostream<charT,Traits>& o,bool multiline=false) const
  {
        return range_print_on(o,this->begin(),end(),DefaultWriter(),multiline);
  }

  template <class charT, class Traits, class Writer >
        std::ios_base::iostate print_on(std::basic_ostream<charT,Traits>& o,Writer writer,bool multiline=false) const
  {
        return range_print_on(o,this->begin(),end(),writer,multiline);
  }


  // if any element read fails, whole array is clobbered (even if appending!)
  // Reader passed by value, so can't be stateful (unless itself is a pointer to shared state)
template <class charT, class Traits, class Reader>
std::ios_base::iostate get_from(std::basic_istream<charT,Traits>& in,Reader read, bool append=REPLACE)

{
  if (append==REPLACE)
        clear();

#if 1
  // slight optimization from not needing temporary like general output iterator version.
  char c;
  EXPECTCH_SPACE_COMMENT('(');
  for(;;) {
    EXPECTI_COMMENT(in>>c);
          if (c==')') {
                break;
          }
          in.unget();
          push_back(); // was doing push_back_raw, but that's bad - need to default construct some types where reader assumes constructedness.
          if (!(deref(read)(in,back())).good()) {
                //undo_push_back_raw();
                goto fail;
          }
          EXPECTI_COMMENT(in>>c);
          if (c != ',') in.unget();
  }
  Assert(invariant());
  return GENIOGOOD;
fail:
  clear();
  return GENIOBAD;
#else
  std::ios_base::iostate ret=
        range_get_from(in,back_inserter(*this),read);
  if (ret == GENIOBAD)
        clear();
  return ret;
#endif
}

};



// outputs sequence to iterator out, of new indices for each element i, corresponding to deleting element i from an array when remove[i] is true (-1 if it was deleted, new index otherwise), returning one-past-end of out (the return value = # of elements left after deletion)
template <class AB,class ABe,class O>
unsigned new_indices(AB i, ABe end,O out) {
  int f=0;
  while (i!=end)
        *out++ = *i++ ? -1 : f++;
  return f;
};

template <class AB,class O>
unsigned new_indices(AB remove,O out) {
  return new_indices(remove.begin(),remove.end());
}

template <typename T,typename Alloc,class charT, class Traits, class Reader>
//std::ios_base::iostate Array<T,Alloc>::get_from(std::basic_istream<charT,Traits>& in,Reader read)
std::ios_base::iostate get_from_imp(Array<T,Alloc> *a,std::basic_istream<charT,Traits>& in,Reader read)
{
  DynamicArray<T,Alloc> s;
  std::ios_base::iostate ret=s.get_from(in,read);
  s.compact(a);
  s.clear_nodestroy();
  return ret;
}

template <class charT, class Traits,class L,class A>
std::basic_istream<charT,Traits>&
operator >>
 (std::basic_istream<charT,Traits>& is, Array<L,A> &arg)
{
        return gen_extractor(is,arg,DefaultReader<L>());
}

template <class charT, class Traits,class L,class A>
std::basic_istream<charT,Traits>&
operator >>
 (std::basic_istream<charT,Traits>& is, DynamicArray<L,A> &arg)
{
        return gen_extractor(is,arg,DefaultReader<L>());
}

template <class charT, class Traits,class L,class A>
std::basic_ostream<charT,Traits>&
operator <<
 (std::basic_ostream<charT,Traits>& os, const Array<L,A> &arg)
{
        return gen_inserter(os,arg);
}


template <class charT, class Traits,class L,class A>
std::basic_ostream<charT,Traits>&
operator <<
 (std::basic_ostream<charT,Traits>& os, const DynamicArray<L,A> &arg)
{
        return gen_inserter(os,arg);
}

#if 1
#define ARRAYEQIMP \
  if (l.size() != r.size()) return false; \
  typename L::const_iterator il=l.begin(),iend=l.end(); \
  typename R::const_iterator ir=r.begin(); \
  while (il!=iend) \
        if (!(*il++ == *ir++)) return false; \
  return true;

#else

#define ARRAYEQIMP return std::equal(l.begin(),l.end(),r.begin());

#endif

template<class Lt,class A,class L2,class A2>
bool operator ==(const DynamicArray<Lt,A> &l, const DynamicArray<L2,A2> &r)
{
  typedef DynamicArray<Lt,A> L;
  typedef DynamicArray<L2,A2> R;
ARRAYEQIMP
}


template<class Lt,class A,class L2,class A2>
bool operator ==(const DynamicArray<Lt,A> &l, const Array<L2,A2> &r)
{
  typedef DynamicArray<Lt,A> L;
  typedef Array<L2,A2> R;
ARRAYEQIMP
}

template<class Lt,class A,class L2,class A2>
bool operator ==(const Array<Lt,A> &l, const DynamicArray<L2,A2> &r)
{
  typedef Array<Lt,A> L;
  typedef DynamicArray<L2,A2> R;
ARRAYEQIMP
}


template<class Lt,class A,class L2,class A2>
bool operator ==(const Array<Lt,A> &l, const Array<L2,A2> &r)
{
  typedef Array<Lt,A> L;
  typedef Array<L2,A2> R;
  ARRAYEQIMP
}




#ifdef TEST
bool rm1[] = { 0,1,1,0,0,1,1 };
bool rm2[] = { 1,1,0,0,1,0,0 };
int a[] = { 1,2,3,4,5,6,7 };
int a1[] = { 1, 4, 5 };
int a2[] = {3,4,6,7};
#include <algorithm>
#include <iterator>
struct plus_one_reader {
    typedef int value_type;
      template <class charT, class Traits>
        std::basic_istream<charT,Traits>&
         operator()(std::basic_istream<charT,Traits>& in,int &l) const {
          in >> l;
          ++l;
          return in;
         }
};
BOOST_AUTO_UNIT_TEST( dynarray )
{
    {
        FixedArray<FixedArray<int> > aa,ba;
        string sa="(() (1) (1 2 3) () (4))";
        BOOST_CHECK(test_extract_insert(sa,aa));
        IndirectReader<plus_one_reader> reader;
        istringstream ss(sa);

        ba.get_from(ss,reader);

//        DBP(ba);
        BOOST_REQUIRE(aa.size()==5);
        BOOST_CHECK(aa[2].size()==3);
        BOOST_REQUIRE(ba.size()==5);
        BOOST_CHECK(ba[2].size()==3);        
        BOOST_CHECK(aa[1][0]==1);
        BOOST_CHECK(ba[1][0]==2);
    }
  {
        DynamicArray<int> a;
        a.at_grow(5)=1;
        BOOST_CHECK(a.size()==5+1);
        BOOST_CHECK(a[5]==1);
        for (int i=0; i < 5; ++i)
          BOOST_CHECK(a.at(i)==0);
  }
  const int sz=7;
  {
  DynamicArray<int> a(sz);
  a.push_back(sz,sz*3);
  BOOST_CHECK(a.size() == sz*3);
  BOOST_CHECK(a.capacity() >= sz*3);
  BOOST_CHECK(a[sz]==sz);
  }

  {
  DynamicArray<int> a(sz*3,sz);
  BOOST_CHECK(a.size() == sz*3);
  BOOST_CHECK(a.capacity() == sz*3);
  BOOST_CHECK(a[sz]==sz);
  }

  using namespace std;
  Array<int> aa(sz);
  BOOST_CHECK(aa.capacity() == sz);
  DynamicArray<int> da;
  DynamicArray<int> db(sz);
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
  for (int i=0;i<sz;++i) {
    BOOST_CHECK(a[i]==aa.at(i));
        BOOST_CHECK(a[i]==da.at(i));
        BOOST_CHECK(a[i]==db(i));
  }
  BOOST_CHECK(da==aa);
  BOOST_CHECK(db==aa);
  const int sz1=3,sz2=4;;
  da.removeMarked(rm1); // removeMarked
  BOOST_REQUIRE(da.size()==sz1);
  for (int i=0;i<sz1;++i)
    BOOST_CHECK(a1[i]==da[i]);
  db.removeMarked(rm2);
  BOOST_REQUIRE(db.size()==sz2);
  for (int i=0;i<sz2;++i)
        BOOST_CHECK(a2[i]==db[i]);
  Array<int> d1map(sz),d2map(sz);
  BOOST_CHECK(3==new_indices(rm1,rm1+sz,d1map.begin()));
  BOOST_CHECK(4==new_indices(rm2,rm2+sz,d2map.begin()));
  int c=0;
  for (unsigned i=0;i<d1map.size();++i)
        if (d1map[i]==-1)
          ++c;
        else
          BOOST_CHECK(da[d1map[i]]==aa[i]);
  BOOST_CHECK(c==4);
  db(10)=1;
  BOOST_CHECK(db.size()==11);
  BOOST_CHECK(db.capacity()>=11);
  BOOST_CHECK(db[10]==1);
  aa.dealloc();

  string emptya=" ()";
  string emptyb="()";
  {
        Array<int> a;
        DynamicArray<int> b;
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

#define EQIOTEST(A,B)  { A<int> a;B<int> b;stringstream o;istringstream isa(sa);isa >> a;\
  o << a;BOOST_CHECK(o.str() == sb);o >> b;BOOST_CHECK(a==b);}

  EQIOTEST(Array,Array)
        EQIOTEST(Array,DynamicArray)
        EQIOTEST(DynamicArray,Array)
        EQIOTEST(DynamicArray,DynamicArray)
}
#endif

#endif
