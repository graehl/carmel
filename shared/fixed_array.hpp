#ifndef GRAEHL_SHARED__FIXED_ARRAY_HPP
#define GRAEHL_SHARED__FIXED_ARRAY_HPP

//only for types where you can use memcpy to move/swap (thus, also more efficient).

//FIXME: const safeness for contents e.g. a[1] a.at(1) return const ref if a const?

// For (MEMCPY-able-to-move) types *only* (e.g. plain old data)  constructor/destructor that do things is fine, as long as nothing goes wrong when relocating the object to a diff address by just moving the bytes with no notification, e.g. circularly linked head node as a member will fail badly
// Array encapsulates a region of memory and doesn't own its own storage ... you can create subarrays of arrays that use the same storage.  you could implement vector or string on top of it.  it does take an allocator argument and has methods alloc, dealloc, re_alloc (realloc is a macro in MS, for shame), which need not be used.  as a rule, nothing writing an Array ever deallocs old space automatically, since an Array might not have alloced it.
// dynamic_array extends an array to allow it to be grown efficiently one element at a time (there is more storage than is used) ... like vector but better for MEMCPY-able stuff
// note that dynamic_array may have capacity()==0 (though using push_back and array(i) will allocate it as necessary)

//#include "config.h"

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <string>
#include <cstring>
#include <new>
#include <cassert>
#include <memory>
#include <cstddef>
#include <stdexcept>
#include <iostream>
#include <graehl/shared/stream_util.hpp>
#include <graehl/shared/genio.h>
#include <graehl/shared/byref.hpp>
#include <graehl/shared/funcs.hpp>
#include <graehl/shared/io.hpp>
#include <graehl/shared/stackalloc.hpp>
#include <graehl/shared/simple_serialize.hpp>
#include <graehl/shared/function_output_iterator.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/iterator/reverse_iterator.hpp>
#include <vector>
#include <algorithm>
#include <iterator>
#include <boost/config.hpp>

#include <graehl/shared/array.hpp>
#include <graehl/shared/hash_functions.hpp>

#ifdef GRAEHL_TEST
# include <graehl/shared/test.hpp>
# define GRAEHL__DYNAMIC_ARRAY_EXTRA_ASSERT
#endif

#ifdef GRAEHL__DYNAMIC_ARRAY_EXTRA_ASSERT
# define dynarray_assert(x) assert(x)
#else
# define dynarray_assert(x)
#endif
//#include <boost/type_traits.hpp>

namespace graehl {

template <class T,class A> class array;

template <typename T,typename Alloc,class charT, class Traits, class Reader>
//std::ios_base::iostate array<T,Alloc>::read(std::basic_istream<charT,Traits>& in,Reader read)
std::ios_base::iostate read_imp(array<T,Alloc> *a,std::basic_istream<charT,Traits>& in,Reader read,unsigned reserve=1000);


// doesn't manage its own space (use alloc(n) and dealloc() yourself).  0 length
// allowed.  you must construct and deconstruct elements yourself.  raw dynamic
// uninitialized (classed) storage array copy constructor is very fast, of
// course.  as long as you don't use any allocation, you can think of this as an
// array-substring (for contiguous things)
template <typename T,typename Alloc=std::allocator<T> > class array : protected Alloc {
 public:
    typedef T value_type;
    typedef value_type *iterator;
    typedef value_type const* const_iterator;
    typedef T &reference;
    typedef const T& const_reference;
    typedef unsigned size_type;
    typedef boost::reverse_iterator<iterator> reverse_iterator;
    typedef boost::reverse_iterator<const_iterator> const_reverse_iterator;
protected:
    //unsigned int space;
    T *vec;
    T *endspace;
public:
  friend std::size_t hash_value(array const& x) {
    return boost_hash_range(x.begin(),x.end());
  }
    static inline size_type bytes(size_type n)
    {
        return sizeof(T)*n;
    }
    BOOST_STATIC_CONSTANT(bool,REPLACE=0);
    BOOST_STATIC_CONSTANT(bool,APPEND=1);

    BOOST_STATIC_CONSTANT(bool,BRIEF=0);
    BOOST_STATIC_CONSTANT(bool, MULTILINE=1);

//    BOOST_STATIC_CONSTANT(bool,DUMMY=0); // msvc++ insists on amibuity between template Writer print and bool 2nd arg ...

    T & at(unsigned int index) const { // run-time bounds-checked
        T *r=vec+index;
        if (!(r < end()) )
            throw std::out_of_range(std::string("array access out of bounds with index=").append(boost::lexical_cast<std::string>(index)));
        return *r;
    }
    bool exists(unsigned index) const
    {
        return begin()+index < end();
    }
    //TODO: const/nonconst ref return
    T & operator[] (unsigned int index) const {
        dynarray_assert(vec+index < end());
        return vec[index];
    }
    T & operator() (unsigned int index) const {
        dynarray_assert(vec+index < end());
        return vec[index];
    }

//    operator T*() const { return vec; }
    bool invariant() const {
        return vec >= endspace;
    }
    typedef array<T,Alloc> self_type;
//FIXME: swap allocator if it has state?
    void swap(array<T,Alloc> &a) throw() {
        /*
        self_type t;
        memcpy(&t,this,sizeof(self_type));
        memcpy(this,&a,sizeof(self_type));
        memcpy(&a,&t,sizeof(self_type));
        */
        T *tvec=vec;
        T *tendspace=endspace;
        vec=a.vec;
        endspace=a.endspace;
        a.vec=tvec;
        a.endspace=tendspace;
    }
    inline friend void swap(self_type &a,self_type &b) throw()
    {
        a.swap(b);
    }
    array<T,Alloc> substr(unsigned start) const
    {
        return substr(start,size());
    }

    array<T,Alloc> substr(unsigned start,unsigned end) const
    {
        dynarray_assert(begin()+start <= endspace && begin()+end <= endspace);
        return array<T,Alloc>(begin()+start,begin()+end);
    }

    void construct() {
        for (T *p=vec;p!=endspace;++p)
            new(p) T();
    }
    template <class V>
    void construct(const V& val) {
        for (T *p=vec;p!=endspace;++p)
            new(p) T(val);
    }

    const T& front()  const {
        dynarray_assert(size());
        return *begin();
    }
    const T& back() const {
        dynarray_assert(size());
        return *(end()-1);
    }

    T& front() {
        dynarray_assert(size());
        return *begin();
    }
    T& back() {
        dynarray_assert(size());
        return *(end()-1);
    }

    template <class charT, class Traits>
    std::basic_ostream<charT,Traits>& print(std::basic_ostream<charT,Traits>& o,bool multiline=false,bool dummy=false,bool dummy2=false) const
        {
            return range_print(o,begin(),end(),DefaultWriter(),multiline);
        }

    typedef void has_print_writer;
    template <class charT, class Traits, class Writer >
    std::basic_ostream<charT,Traits>& print(std::basic_ostream<charT,Traits>& o,Writer w,bool multiline=false) const
        {
            return range_print(o,begin(),end(),w,multiline);
        }


// Reader passed by value, so can't be stateful (unless itself is a pointer to shared state)
    template <class T2,class Alloc2, class charT, class Traits, class Reader> friend
    std::ios_base::iostate read_imp(array<T2,Alloc2> *s,std::basic_istream<charT,Traits>& in,Reader read,unsigned reserve);
    template <class L,class A> friend
    void read(std::istream &in,array<L,A> &x,StackAlloc &a);// throw(genio_exception);


    template <class charT, class Traits, class Reader>
    std::ios_base::iostate read(std::basic_istream<charT,Traits>& in,Reader read,unsigned reserve=1000) {
        return read_imp(this,in,read,reserve);
    }

    template <class charT, class Traits>
    std::ios_base::iostate read(std::basic_istream<charT,Traits>& in) {
        return read(in,DefaultReader<T>());
    }

    bool empty() const {
        return vec==endspace;
    }
    void dealloc() {
        int cap=capacity();
        if (cap) {
            dynarray_assert(vec);
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
      std::memcpy(to,vec,bytes(n));
    }

    void copyto(T *to) {
        copyto(to,capacity());
    }

    // doesn't copy old elements like dynamicarray does
    void alloc(unsigned sp) {
        if(sp) {
            vec=this->allocate(sp);
            endspace=vec+sp;
        } else { // necessary because valgrind complained about uninitialized value of vec
            endspace=vec=0;
        }
    }
    void re_alloc(unsigned sp) {
//      if (sp == space) return;
        dealloc();
        alloc(sp);
    }

    // okay to shallow copy since we don't automatically destroy anything
    template <class Alloc2>
    array(const array<T,Alloc2>&o) : vec(o.vec),endspace(o.endspace) {}

    template <class Alloc2>
    const self_type & operator =(const array<T,Alloc2>&o) {
        vec=o.vec;
        endspace=o.endspace;
        return *this;
    }

    explicit array(const char *c) {
        std::istringstream(c) >> *this;
    }

    // create array reference to a STL vector - note: I *believe* stdc++ requires storage be contiguous, i.e. iterators be pointers ... if not, nonportable
    explicit array(std::vector<T> &vec)
    {
        init(vec);
    }
    void init(std::vector<T> &vector)
    {
        vec=&(vector.front());
        set_capacity(vector.size());
    }


    //FIXME: unsafe: etc. I'm relying on contiguousness of std::vector storage
    template <class A2>
    array(std::vector<T,A2> &source,unsigned starti,unsigned endi)
    {
        dynarray_assert(endi <= source.size());
        vec=&(source[starti]);
        endspace=&(source[endi]);
    }

    template <class A2>
    array(typename std::vector<T,A2>::const_iterator begin,typename std::vector<T,A2>::const_iterator end) : vec(const_cast<T *>(&*begin)), endspace(const_cast<T *>(&*end)) { }

    array(const T *begin, const T *end) : vec(const_cast<T *>(begin)), endspace(const_cast<T *>(end)) { }
    array(const T* buf,unsigned sz) : vec(const_cast<T *>(buf)), endspace(buf+sz) {}
  array() : vec(0),endspace(0) {  }
    explicit array(unsigned sp) { alloc(sp); }
    array(unsigned sp, const Alloc &a): Alloc(a) { alloc(sp); }
    template<class I>
    array(unsigned n,I begin) { // copy up to n
        alloc(n);
        nonstd::uninitialized_copy_n(begin, n, vec);
    }

    unsigned capacity() const { return (unsigned)(endspace-vec); }
    unsigned size() const { return capacity(); }
    T * begin() { return vec; }
    T * end() {       return endspace;      }
    reverse_iterator rbegin()
    {
        return reverse_iterator(end());
    }
    reverse_iterator rend()
    {
        return reverse_iterator(begin());
    }

    const T* begin() const {
        return vec;
    }
    const T* end() const {
        return endspace;
    }

    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(end());
    }
    const_reverse_iterator rend() const
    {
        return const_reverse_iterator(begin());
    }

    const T* const_begin() const {
        return vec;
    }
    const T* const_end() const {
        return endspace;
    }
    unsigned int index_of(T *t) const {
        dynarray_assert(t>=begin() && t<end());
        return (unsigned int)(t-vec);
    }
    ///XXX: must always properly set_capacity AFTER set_begin since this invalidates capacity
    void set_begin(T* buf) {
        //unsigned cap=capacity();
        vec=buf;
        //set_capacity(cap);
    }
    void set_capacity(unsigned int newCap) { endspace=vec+newCap; }
    TO_OSTREAM_PRINT
    FROM_ISTREAM_READ
};

// frees self automatically - WARNING - doesn't copy contents!  inits self and destroys self as well
template <typename T,typename Alloc=std::allocator<T> > class auto_array : public array<T,Alloc> {
public:
    typedef array<T,Alloc> Super;
    explicit auto_array(unsigned sp=0) : Super(sp) {
    }
    ~auto_array() {
        this->dealloc();
    }
    void clear()
    {
        this->dealloc();
    }

    void alloc(unsigned sp) {
        Super::re_alloc(sp);
    }
    template <class charT, class Traits, class Reader>
    std::ios_base::iostate read(std::basic_istream<charT,Traits>& in,Reader read) {
        this->dealloc();
        return read_imp(this,in,read);
    }
    template <class charT, class Traits>
    std::ios_base::iostate read(std::basic_istream<charT,Traits>& in) {
        this->dealloc();
        return read_imp(this,in,DefaultReader<T>(),1000);
//        return read(in,);
    }

protected:
    auto_array(auto_array<T,Alloc> &a) : Super(a.capacity()){
    }
private:
    void operator=(const array<T,Alloc> &a) {
        dynarray_assert(0);
    }
 public:
    typedef auto_array<T,Alloc> self_type;
    TO_OSTREAM_PRINT
    FROM_ISTREAM_READ
    inline friend void swap(self_type &a,self_type &b) throw()
    {
        a.swap(b);
    }
};

template <class T,class Alloc>
inline bool operator < (array<T,Alloc> &a,array<T,Alloc> &b)
{
    return std::lexicographical_compare(a.begin(),a.end(),b.begin(),b.end());
}


template <typename T,typename Alloc=std::allocator<T> > class dynamic_array;

// FIXME: drop template copy constructor since it screws with one arg size constructor
// frees self automatically; inits/destroys/copies just like dynamic_array but can't be resized.
template <typename T,typename Alloc=std::allocator<T> > class fixed_array : public auto_array<T,Alloc> {
public:
    typedef auto_array<T,Alloc> Super;
  typedef array<T,Alloc> Array;
    explicit fixed_array() : Super(0) {  }
    typedef unsigned size_type;
    explicit fixed_array(size_type sp) : Super(sp) {
        this->construct();
    }
    fixed_array(T const& t,size_type sp) : Super(sp) {
        this->construct(t);
    }
    ~fixed_array() {
        this->destroy();
        //~Super(); // happens implicitly!
    }
    void clear()
    {
        this->destroy();
        this->dealloc();
    }

    // pre: capacity is 0
    void init(size_type sp)
    {
        assert(this->capacity()==0);
        this->alloc(sp);
        this->construct();
    }
    void init(size_type sp,T const&t)
    {
        assert(this->capacity()==0);
        this->alloc(sp);
        this->construct(t);
    }

    void reinit(size_type sp)
    {
        if (sp==this->capacity()) {
            this->destroy();
            this->construct();
        } else {
            clear();
            init(sp);
        }
    }
    void reinit(size_type sp,const T& t) {
        if (sp==this->capacity()) {
            this->destroy();
            this->construct(t);
        } else {
            clear();
            init(sp,t);
        }
    }
    void reinit_nodestroy(size_type sp)
    {
        if (sp==this->capacity()) {
            this->construct();
        } else {
            this->dealloc();
            init(sp);
        }
    }
    void reinit_nodestroy(size_type sp,const T& t) {
        if (sp==this->capacity()) {
            this->construct(t);
        } else {
            this->dealloc();
            init(sp,t);
        }
    }

    template <class charT, class Traits, class Reader>
    std::ios_base::iostate read(std::basic_istream<charT,Traits>& in,Reader read) {
        this->destroy();
        this->dealloc();
        return read_imp((Array *)this,in,read);
    }
    template <class charT, class Traits>
    std::ios_base::iostate read(std::basic_istream<charT,Traits>& in) {
        return read(in,DefaultReader<T>());
    }

    void uninit_copy_from(const T* b,const T* e) {
        dynarray_assert(e-b == this->capacity());
        std::uninitialized_copy(b,e,this->begin());
    }
    template <class V>
    void uninit_copy_from(V const& v)
    {
        uninit_copy_from(v.begin(),v.end());
    }
    void copy_from(const T* b,const T* e)
    {
        this->destroy();
        uninit_copy_from(b,e);
    }
    template <class V>
    void copy_from(V const& v)
    {
        copy_from(v.begin(),v.end());
    }

    fixed_array(const array<T,Alloc> &a) : Super(a.capacity()) {
//        DBPC("fixed_array copy",a);
        uninit_copy_from(a.begin(),a.end());
    }
    fixed_array(const auto_array<T,Alloc> &a) : Super(a.capacity()) {
        //       DBPC("fixed_array copy",a);
        uninit_copy_from(a.begin(),a.end());
    }
    template <class A>
    fixed_array(const dynamic_array<T,A> &a) : Super(a.capacity()) {
        // DBPC("fixed_array copy",a);
        uninit_copy_from(a.begin(),a.end());
    }
    fixed_array(const std::vector<T> &a) : Super(a.capacity()) {
        // DBPC("fixed_array copy",a);
        uninit_copy_from(&*a.begin(),&*a.end());
    }

    //F=fixed_array<T,Alloc>

// having problems with ambiguous constructor?  make sure you pass an *unsigned* int if you want to specify # of elements.  I don't know how to make this constructor exclude integral types, well, I could do it with type traits and a second param is_container<F> *dummy=0
    /*
    template <class F>
    explicit fixed_array(const F &a) : Super(a.size()) {
        //    DBPC("fixed_array copy",a);
        uninit_copy_from(a.begin(),a.end());
    }
    */
    fixed_array(const fixed_array<T,Alloc>  &a) : Super(a.size()) {
        //    DBPC("fixed_array copy",a);
        uninit_copy_from(a.begin(),a.end());
    }
    typedef fixed_array<T,Alloc> self_type;
    TO_OSTREAM_PRINT
    FROM_ISTREAM_READ
};

#if 0
template <class charT, class Traits,class L,class A>
std::basic_istream<charT,Traits>&
operator >>
(std::basic_istream<charT,Traits>& is, array<L,A> &arg)
{
    return gen_extractor(is,arg);
}

template <class charT, class Traits,class L,class A>
std::basic_ostream<charT,Traits>&
operator <<
    (std::basic_ostream<charT,Traits>& os, const array<L,A> &arg)
{
    return gen_inserter(os,arg);
}

template <class charT, class Traits,class L,class A>
std::basic_istream<charT,Traits>&
operator >>
(std::basic_istream<charT,Traits>& is, auto_array<L,A> &arg)
{
    return gen_extractor(is,arg);
}

template <class charT, class Traits,class L,class A>
std::basic_istream<charT,Traits>&
operator >>
(std::basic_istream<charT,Traits>& is, fixed_array<L,A> &arg)
{
    return gen_extractor(is,arg);
}
#endif
}

//FIXME: overriding std::swap is technically illegal, but this allows some dumb std::sort to swap, which it should have found in graehl::swap by arg. dependent lookup.
namespace std {
template <class T,class A>
void swap(graehl::array<T,A> &a,graehl::array<T,A> &b) throw()
{
    a.swap(b);
}
template <class T,class A>
void swap(graehl::auto_array<T,A> &a,graehl::auto_array<T,A> &b) throw()
{
    a.swap(b);
}
template <class T,class A>
void swap(graehl::fixed_array<T,A> &a,graehl::fixed_array<T,A> &b) throw()
{
    a.swap(b);
}
}

#include <graehl/shared/dynarray.h>
// needed for read_imp for now
#endif
