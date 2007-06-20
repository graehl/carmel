// like std::vector but exposes contiguous-array-implementation  - only for types where you can use memcpy to move/swap (thus, also more efficient).  now also includes fixed size arrays.
// historical justification: when Carmel was first written, STL wasn't supported by gcc.
#ifndef GRAEHL__SHARED__DYNARRAY_H
#define GRAEHL__SHARED__DYNARRAY_H
//FIXME: const safeness for contents e.g. a[1] a.at(1) return const ref if a const?

// For plain old data (MEMCPY-able-to-move) types only!
// (MEMCPY copyable (i.e. no external resources owned, i.e. no real destructor) is not assumed although a is_pod template might be nice)
// Array encapsulates a region of memory and doesn't own its own storage ... you can create subarrays of arrays that use the same storage.  you could implement vector or string on top of it.  it does take an allocator argument and has methods alloc, dealloc, re_alloc (realloc is a macro in MS, for shame), which need not be used.  as a rule, nothing writing an Array ever deallocs old space automatically, since an Array might not have alloced it.
// dynamic_array extends an array to allow it to be grown efficiently one element at a time (there is more storage than is used) ... like vector but better for MEMCPY-able stuff
// note that dynamic_array may have capacity()==0 (though using push_back and array(i) will allocate it as necessary)

//#include "config.h"

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <string>
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
#include <graehl/shared/function_output_iterator.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>
#include <algorithm>
#include <iterator>
#include <boost/config.hpp>
#include <graehl/shared/array.hpp>

#ifdef TEST
# include <graehl/shared/test.hpp>
#endif

#ifdef GRAEHL__DYNAMIC_ARRAY_EXTRA_ASSERT
# define dynarray_assert(x) assert(x)
#else 
# define dynarray_assert(x)
#endif 
//#include <boost/type_traits.hpp>

namespace graehl {


// doesn't manage its own space (use alloc(n) and dealloc() yourself).  0 length
// allowed.  you must construct and deconstruct elements yourself.  raw dynamic
// uninitialized (classed) storage array copy constructor is very fast, of
// course.  as long as you don't use any allocation, you can think of this as an
// array-substring (for contiguous things)
template <typename T,typename Alloc=std::allocator<T> > class array : protected Alloc {
 public:
    typedef T value_type;
    typedef value_type *iterator;
    typedef const value_type *const_iterator;
    typedef T &reference;
    typedef const T& const_reference;
    typedef unsigned size_type;
    
protected:
    //unsigned int space;
    T *vec;
    T *endspace;
public:
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
    T & operator[] (unsigned int index) const {
        dynarray_assert(vec+index < end());
        return vec[index];
    }

//    operator T*() const { return vec; }
    bool invariant() const {
        return vec >= endspace;
    }
    typedef array<T,Alloc> self_type;
//!FIXME: does this swap allocator base?
    void swap(array<T,Alloc> &a) {
        self_type t;
        memcpy(&t,this,sizeof(self_type));
        memcpy(this,&a,sizeof(self_type));
        memcpy(&a,&t,sizeof(self_type));
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
    std::ios_base::iostate read_imp(array<T2,Alloc2> *s,std::basic_istream<charT,Traits>& in,Reader read);
    template <class L,class A> friend
    void read(std::istream &in,array<L,A> &x,StackAlloc &a);// throw(genio_exception);


    template <class charT, class Traits, class Reader>
    std::ios_base::iostate read(std::basic_istream<charT,Traits>& in,Reader read) {
        return read_imp(this,in,read);
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
        memcpy(to,vec,sizeof(T)*n);
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
    explicit array(unsigned sp=4) { alloc(sp); }
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
    const T* begin() const {
        return vec;
    }
    const T* end() const {
        return endspace;
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

// frees self automatically - WARNING - doesn't copy contents!
template <typename T,typename Alloc=std::allocator<T> > class auto_array : public array<T,Alloc> {
public:
    typedef array<T,Alloc> Super;
    explicit auto_array(unsigned sp=0) : Super(sp) { }
    ~auto_array() {
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
        return read(in,DefaultReader<T>());
    }

//!FIXME: doesn't swap allocator base
    //    void swap(array<T,Alloc> &a) {dynarray_assert(0);}
//FIXME: should only work for dynarray,fixedarray,autoarray
/*    void swap(array<T,Alloc> &a) {
        array<T,Alloc>::swap(a);
        }*/
protected:
    auto_array(auto_array<T,Alloc> &a) : Super(a.capacity()){
    }
private:
    void operator=(const array<T,Alloc> &a) {
        dynarray_assert(0);
    }
    typedef auto_array<T,Alloc> self_type;
    TO_OSTREAM_PRINT
    FROM_ISTREAM_READ
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
    explicit fixed_array(std::size_t sp=0) : Super(sp) {
        this->construct();
    }
    ~fixed_array() {
        this->destroy();
        //~Super(); // happens implicitly!
    }
    template <class charT, class Traits, class Reader>
    std::ios_base::iostate read(std::basic_istream<charT,Traits>& in,Reader read) {
        this->destroy();
        this->dealloc();
        return read_imp(this,in,read);
    }
    template <class charT, class Traits>
    std::ios_base::iostate read(std::basic_istream<charT,Traits>& in) {
        return read(in,DefaultReader<T>());
    }

    void uninit_copy_from(const T* b,const T* e) {
        dynarray_assert(e-b == this->capacity());
        std::uninitialized_copy(b,e,this->begin());
        //memcpy(begin(),b,e-b);
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


// caveat:  cannot hold arbitrary types T with self or mutual-pointer refs; only works when memcpy can move you
// FIXME: possible for this to not be valid for any object with a default constructor :-(
template <typename T,class Alloc> class dynamic_array : public array<T,Alloc> {
    typedef dynamic_array<T,Alloc> self_type;
    //unsigned int sz;
    T *endv;
    typedef array<T,Alloc> Base;
private:
    dynamic_array& operator = (const dynamic_array &a){std::cerr << "unauthorized assignment of a dynamic array\n";dynarray_assert(0);}
    void swap(array<T,Alloc> &a) {dynarray_assert(0);}
public:
    explicit dynamic_array (const char *c) {
        std::istringstream(c) >> *this;
    }

    // creates vector with CAPACITY for sp elements; size()==0; doesn't initialize (still use push_back etc)
    explicit dynamic_array(unsigned sp = 4) : array<T,Alloc>(sp), endv(Base::vec) { dynarray_assert(this->invariant()); }

    // creates vector holding sp copies of t; does initialize
    template <class V>
    explicit dynamic_array(unsigned sp,const V& t) : array<T,Alloc>(sp) {
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

    void reinit(unsigned sp,const T& t=T()) {
        clear();
        reserve(sp);
        T *real_es=this->endspace;
        endv=this->endspace=sp;
        array<T,Alloc>::construct(t);
        this->endspace=real_es;
    }
    void reinit_nodestroy(unsigned sp,const T& t=T()) {
        reserve(sp);
        T *real_es=this->endspace;
        endv=this->endspace=this->vec+sp;
        array<T,Alloc>::construct(t);
        this->endspace=real_es;
    }

    dynamic_array(const dynamic_array &a) : array<T,Alloc>(a.size()) {
//      unsigned sz=a.size();
        //alloc(sz);
//      memcpy(this->vec,a.vec,sizeof(T)*sz);
        std::uninitialized_copy(a.begin(),a.end(),this->begin());
        endv=this->endspace;
        dynarray_assert(this->invariant());
    }

    template<class RIT>
    dynamic_array(typename boost::disable_if<boost::is_arithmetic<RIT>,RIT>::type const& a,RIT const& b) : array<T,Alloc>(b-a) 
    {
        std::uninitialized_copy(a,b,this->begin());
        endv=this->endspace;
        dynarray_assert(this->invariant());
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

    const T* end() const { // array code that uses vec+space for boundschecks is duplicated below
        dynarray_assert(this->invariant());
        return endv;
    }
    T* end()  { // array code that uses vec+space for boundschecks is duplicated below
        dynarray_assert(this->invariant());
        return endv;
    }
    const T* const_end() const {
        return endv;
    }
    typedef typename array<T,Alloc>::iterator iterator;
    // move a chunk [i,end()) off the back, leaving the vector as [vec,i)
    void move_rest_to(T *to,iterator i) {
        dynarray_assert(i >= this->begin() && i < end());
        copyto(to,i,this->end()-i);
        endv=i;
    }


    T & at(unsigned int index) const { // run-time bounds-checked
        T *r=this->vec+index;
        if (!(r < end()) )
            throw std::out_of_range("dynamic_array index out of bounds");
        return *r;
    }
    bool exists(unsigned index) const
    {
        return this->begin()+index < end();
    }

    T & operator[] (unsigned int index) const {
        dynarray_assert(this->invariant());
        dynarray_assert(this->vec+index < end());
        return (this->vec)[index];
    }
    unsigned int index_of(T *t) const {
        dynarray_assert(t>=this->begin() && t<end());
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
    
    void push_back_n(const T& val,unsigned n)
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
        if ( endv >= this->endspace )
            if (this->vec == this->endspace )
                realloc_up(4);
            else
                realloc_up(this->capacity()*2); // FIXME: 2^31 problem
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
    
    T &at_grow(unsigned index) {
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
    void construct_n_more(unsigned n) 
    {
        Assert(endv+n <= this->endspace);
        while (--n)
            new (endv++) T();
    }

    void resize_up(unsigned new_sz) 
    {
        unsigned sz=size();
        dynarray_assert(new_sz > size());
        realloc_up(new_sz);
        construct_n_more(new_sz-sz);
    }
    
    void realloc_up(unsigned int new_cap) {
        //     we are somehow allowing 0-capacity vectors now?, so add 1
        //if (new_cap==0) new_cap=1;
        unsigned sz=size();
        dynarray_assert(new_cap > this->capacity());
        // may be used when we've increased endv past endspace, in order to fix things
        T *newVec = this->allocate(new_cap); // can throw but we've made no changes yet
        memcpy(newVec, this->vec, sz*sizeof(T));
        dealloc_safe();
        set_begin(newVec);this->set_capacity(new_cap);set_size(sz);
        // caveat:  cannot hold arbitrary types T with self or mutual-pointer refs
    }
    void dealloc_safe() {
        unsigned oldcap=this->capacity();
        if (oldcap)
            this->deallocate(this->vec,oldcap); // can't throw
    }
public:
    void resize(unsigned int newSz) {
        dynarray_assert(invariant());
        //    if (newSz==0) newSz=1;
        unsigned sz=size();
        if (newSz<sz) {
            reduce_size(newSz);
            return;
        }
        if (sz==newSz)
            return;
        resize_up(newSz);
    }
    //iterator_tags<Input> == random_access_iterator_tag
    template <class Input>
    void insert(iterator pos,Input from,Input to) 
    {
        if (pos==end()) {
            append(from,to);
        } else {
            std::size_t n_new=from-to;
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
    template <class Input>
    void append(Input from,Input to) 
    {
        std::size_t n_new=from-to;
        reserve(size()+n_new);
        while (from!=to)
            new(endv++) T(*from++);
//            *endv++=*from++;
        dynarray_assert(invariant());
    }

    // could just use copy, back_inserter
    template <class Input>
    void append_push_back(Input from,Input to) 
    {
        while (from!=to)
            push_back(*from++);
    }
    
    void compact() {
        dynarray_assert(invariant());
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
    void compact(array<T,Alloc> &into) {
        unsigned sz=size();
        into.alloc(sz);
        copyto(into.begin());
    }

    // doesn't dealloc *into
    void compact_giving(array<T,Alloc> &into) {
        unsigned sz=size();
        into.alloc(sz);
        copyto(into.begin());
        clear_nodestroy();
    }

    void reserve(unsigned int newSpace) {
        if (newSpace > this->capacity())
            realloc_up(newSpace);
    }
    void reserve_at_least(unsigned req) {
        unsigned newcap=this->capacity();
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
    unsigned int size() const { return (unsigned)(endv-this->vec); }
    void set_size(unsigned newSz) { endv=this->vec+newSz; dynarray_assert(this->invariant()); }
    void reduce_size(unsigned int n) {
        if (n==0) {
            clear_dealloc();
            return;
        }
        T *end=endv;
        reduce_size_nodestroy(n);
        for (T *i=endv;i<end;++i)
            i->~T();
    }
    void reduce_size_nodestroy(unsigned int n) {
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
        for ( T *i=this->begin();i!=end();++i)
            i->~T();
        clear_nodestroy();
    }
    void clear_dealloc() {
        dynarray_assert(invariant());
        for ( T *i=this->begin();i!=end();++i)
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
    std::ios_base::iostate read(std::basic_istream<charT,Traits>& in,Reader read, bool append=false)

        {
            if (!append)
                clear();

#if 1
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
#else
            //FIXME:
            std::back_insert_iterator<self_type> appender(*this);            
            std::ios_base::iostate ret=
                range_read(in,appender,read);
            if (ret == GENIOBAD)
                clear();
            return ret;
#endif
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

template <typename T,class A>
void push_back(dynamic_array<T,A> &v)
{
    v.push_back();
}



template <typename T,typename Alloc,class charT, class Traits, class Reader>
//std::ios_base::iostate array<T,Alloc>::read(std::basic_istream<charT,Traits>& in,Reader read)
std::ios_base::iostate read_imp(array<T,Alloc> *a,std::basic_istream<charT,Traits>& in,Reader read)
{
    dynamic_array<T,Alloc> s;
    std::ios_base::iostate ret=s.read(in,read);
    s.compact_giving(*a); // transfers to a
    return ret;
}

/*
template <class charT, class Traits,class L,class A>
std::basic_istream<charT,Traits>&
operator >>
(std::basic_istream<charT,Traits>& is, array<L,A> &arg)
{
    return gen_extractor(is,arg);
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
    (std::basic_ostream<charT,Traits>& os, const array<L,A> &arg)
{
    return gen_inserter(os,arg);
}


template <class charT, class Traits,class L,class A>
std::basic_ostream<charT,Traits>&
operator <<
    (std::basic_ostream<charT,Traits>& os, const dynamic_array<L,A> &arg)
{
    return gen_inserter(os,arg);
}
*/
              
#if 1
#define ARRAYEQIMP                                              \
    if (l.size() != r.size()) return false;                     \
    typename L::const_iterator il=l.begin(),iend=l.end();       \
    typename R::const_iterator ir=r.begin();                    \
    while (il!=iend)                                            \
        if (!(*il++ == *ir++)) return false;                    \
    return true;

#else

#define ARRAYEQIMP return std::equal(l.begin(),l.end(),r.begin());

#endif

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
BOOST_AUTO_UNIT_TEST( test_dynarray )
{
    using namespace std;
    {
        const int N=10;

        StackAlloc al;
        int aspace[N];
        al.init(aspace,aspace+N);
        istringstream ina("(1 2 3 4)");
        array<int> aint;
        read(ina,aint,al);
        BOOST_CHECK(aint.size()==4);
        BOOST_CHECK(aint[3]==4);
        BOOST_CHECK(al.top=aspace+4);
    }

    {
        fixed_array<fixed_array<int> > aa,ba;
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
        fixed_array<fixed_array<int> > aa,ba;
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
        dynamic_array<int> a;
        a.at_grow(5)=1;
        BOOST_CHECK(a.size()==5+1);
        BOOST_CHECK(a[5]==1);
        for (int i=0; i < 5; ++i)
            BOOST_CHECK(a.at(i)==0);
    }
    const int sz=7;
    {
        dynamic_array<int> a(sz);
        a.push_back_n(sz,sz*3);
        BOOST_CHECK(a.size() == sz*3);
        BOOST_CHECK(a.capacity() >= sz*3);
        BOOST_CHECK(a[sz]==sz);
    }

    {
        dynamic_array<int> a(sz*3,sz);
        BOOST_CHECK(a.size() == sz*3);
        BOOST_CHECK(a.capacity() == sz*3);
        BOOST_CHECK(a[sz]==sz);
    }

    using namespace std;
    array<int> aa(sz);
    BOOST_CHECK_EQUAL(aa.capacity(),sz);
    dynamic_array<int> da;
    dynamic_array<int> db(sz);
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
    da.removeMarked_nodestroy(rm1); // removeMarked
    BOOST_REQUIRE(da.size()==sz1);
    for (int i=0;i<sz1;++i)
        BOOST_CHECK(a1[i]==da[i]);
    db.removeMarked_nodestroy(rm2);
    BOOST_REQUIRE(db.size()==sz2);
    for (int i=0;i<sz2;++i)
        BOOST_CHECK(a2[i]==db[i]);
    array<int> d1map(sz),d2map(sz);
    BOOST_CHECK(3==new_indices(rm1,rm1+sz,d1map.begin()));
    BOOST_CHECK(4==new_indices(rm2,rm2+sz,d2map.begin()));
    int c=0;
    for (unsigned i=0;i<d1map.size();++i)
        if (d1map[i]==-1)
            ++c;
        else
            BOOST_CHECK(da[d1map[i]]==aa[i]);

// remove_marked_swap
    std::vector<int> ea(aa.begin(),aa.end());
    dynamic_array<int> eb(ea.begin(),ea.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(ea.begin(),ea.end(),aa.begin(),aa.end());
    remove_marked_swap(ea,rm1); // removeMarked
    BOOST_REQUIRE(ea.size()==sz1);
    for (int i=0;i<sz1;++i)
        BOOST_CHECK(a1[i]==ea[i]);
    remove_marked_swap(eb,rm2);
    BOOST_REQUIRE(eb.size()==sz2);
    for (int i=0;i<sz2;++i)
        BOOST_CHECK(a2[i]==eb[i]);

    std::vector<int> o2n(aa.size());
    BOOST_CHECK_EQUAL(indices_after_remove_marked(array_begin(o2n),rm1,7),ea.size());
    for (int i=0;i<aa.size();++i)
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
        array<int> a;
        dynamic_array<int> b;
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

#define EQIOTEST(A,B)  do { A<int> a;B<int> b;stringstream o;istringstream isa(sa);isa >> a;    \
        o << a;BOOST_CHECK(o.str() == sb);o >> b;BOOST_CHECK(a==b);} while(0)

    EQIOTEST(array,array);
    EQIOTEST(array,dynamic_array);
    EQIOTEST(dynamic_array,array);
    EQIOTEST(dynamic_array,dynamic_array);
}
#endif

} //graehl

#endif
