#ifndef GRAEHL__SHARED__TRIANGULAR_ARRAY_HPP
#define GRAEHL__SHARED__TRIANGULAR_ARRAY_HPP

#include <memory>
#include <new>

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#endif

namespace graehl {

/**
 (square) triangular
 m(i,j), 0<=i<j<=max_span

laid out by: j=1, 0<i<j [1 element].
j=2, 0<i<j [2 elements]
j=3 [3 elements]
...
j=max_span [max_span elements]
*/
template <class V, class Alloc=std::allocator<V> >
struct triangular_array : protected Alloc
{
 protected:
    typedef std::allocator<V> alloc_type;
    typedef triangular_array<V,Alloc> self_type;
 public:
    
    typedef V value_type;
    typedef unsigned index_type;
    typedef V* iterator;
    typedef V const* const_iterator;
    
    alloc_type &alloc() { return *this; }
    alloc_type const& alloc() const { return *this; }

    // v(index_type a, index_type b,V &v)
    //TODO: non-const
    template <class Visit>
    void visit_indexed(Visit const& v) const
    {
        /*
        for (index_type a=0;a<m;++a)
            for (index_type b=a+1;b<=m;+=b)
                v(a,b,(*this)(a,b));
        */
        iterator i=begin();
        for (index_type b=1;b<=m;++b)
            for (index_type a=0;a<b;++a)
                v(a,b,*i++);
        assert(i==end());
    }
    
    index_type max_end() const 
    {
        return m;
    }
    
    // TODO: const/nonconst
    V *begin() const
    {
        return vec;
    }

    V *end() const
    {
        return vec+sz;
    }
    
    triangular_array(index_type max_span=0) { set(max_span); }

    static inline std::size_t n_triangle(std::size_t m)
    // note: n_triangle(0)=0, rounding down.  we need this for index.
    {
        return (m*(m+1))/2;
    }

    std::size_t size() const 
    {
        return sz;
    }
    
    void clear() 
    {
        dealloc();
        m=0;
    }

    void reset(index_type max_span=0)
    {
        dealloc();
        set(max_span);
    }
    
    ~triangular_array()
    { clear(); }

    // TODO: const/nonconst
    value_type & operator()(index_type a,index_type b) const
    {
        return vec[index(a,b)];        
    }
    // TODO: const/nonconst
    value_type & operator[](index_type i) const
    {
        return vec[i];
    }
    
    index_type index(index_type a,index_type b) const
    {
        assert(b>0 && a>=0 && b<=m && a<b );
        return n_triangle(b-1)+a;
    }
    
 private:
    V *vec;
    index_type m;
    std::size_t sz;

    void dealloc() 
    {
        if (sz) {
            for (index_type i=0;i<sz;++i)
                vec[i].~V();
            alloc().deallocate(vec,sz);
            sz=0;
        }
    }
    
    void set(index_type max_span) 
    {
        m=max_span;
        sz=n_triangle(m);
        if (m > 0) {
            vec=alloc().allocate(sz);
        }
        try {
            for (index_type i=0;i<sz;++i)
                new(&vec[i]) V();
        } catch(...) {
            alloc().deallocate(vec,sz);
        }
    }
        
    triangular_array(self_type const& o);
    void operator=(self_type const& o);
};


#ifdef GRAEHL_TEST
BOOST_AUTO_TEST_CASE( triangular_array_test )
{
    typedef triangular_array<unsigned> A;
    A m2;
    unsigned zero=0;
    BOOST_CHECK_EQUAL(m2.size(),zero);
    for (unsigned max=0;max<10;++max) {
        A m(max);
        m2.reset(max);
        BOOST_CHECK_EQUAL(m.size(),(max*(max+1))/2);
        BOOST_CHECK_EQUAL(m2.size(),m.size());
        unsigned c=0;
        for (A::iterator i=m.begin(),e=m.end();i!=e;++i) {
            BOOST_CHECK_EQUAL(m2.begin()[c],zero);
            BOOST_CHECK_EQUAL(*i,zero);
            *i=++c;
        }
        c=0;
        for (unsigned b=1;b<=max;++b) {
            for (unsigned a=0;a<b;++a) {
                BOOST_CHECK_EQUAL(m.index(a,b),c);
                ++c;
                BOOST_CHECK_EQUAL(m(a,b),c);
            }
        }
    }
    m2.clear();
    BOOST_CHECK_EQUAL(m2.size(),zero);
}
#endif

}//ns

#endif
