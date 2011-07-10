//you create a fix-sized "stack" and allocate objects by a stack-like discipline without any bookkeeping.  used by swapbatch.hpp.  similar to packedalloc.hpp but without the ability to chain together lists of chunks - just a single chunk.
#ifndef STACKALLOC_HPP
#define STACKALLOC_HPP

#include <stdexcept>
#include <graehl/shared/align.hpp>
#include <graehl/shared/verbose_exception.hpp>

// Iis it safe to align once then repeatedly alloc<T> if e.g. sizeof(T) is 6 and
//alignment is 4 ... does that mean you need to increment pointer by 8?  or does
//T* x; x++ inc by 8 for you?  I think so, because if that alignment was
//required, then sizeof(T) would be 8, not 6.

// i.e. T* x; ++x or --x always preserves alignment.

namespace graehl {

struct StackAlloc
{
    // throws when allocation fails (I don't let you check ahead of time)
    struct Overflow : public verbose_exception
    {
        VERBOSE_EXCEPTION_WRAP(Overflow)
    };

    void *top; // assuming alignment, next thing to be allocated starts here
    void *end; // if top reaches or exceeds end, alloc fails
    int remain() const
    {
        return ((char *)end)-((char*)top);
    }
    template <class T>
    void align()
    {
/*        unsigned & ttop(*(unsigned *)&top); //FIXME: do we need to warn compiler about aliasing here?
        const unsigned align=boost::alignment_of<T>::value;
        const unsigned align_mask=(align-1);
        unsigned diff=align_mask & ttop; //= align-(ttop&align_mask)
        if (diff) {
            ttop |= align_mask; // = ttop + diff - 1
            ++ttop;
            }*/
        top=graehl::align_up((T*)top);
    }
    template <class T>
    T* aligned_alloc(unsigned n=1) /* throw(StackAlloc::Overflow) */
    {
        this->align<T>();
        return alloc<T>(n);
    }
    bool overfull() const
    {
        return top > end;
    }
    bool full() const
    {
        return top >= end;  // important: different size T will be allocated, and
                         // no guarantee that all T divide space equally (even
                         // for alloc(1)
    }
    template <class T>
    T* next() const {
        Assert(graehl::is_aligned((T*)top));
        return ((T*)top);
    }
    template <class T>
    T* aligned_next() {
        top=graehl::align_up((T*)top);
        return ((T*)top);
    }
    template <class T>
    T* alloc(unsigned n=1) /* throw(StackAlloc::Overflow) */
    {
        T*& ttop(*(T**)&top);
        Assert(graehl::is_aligned(ttop));
        T* ret=ttop;
        ttop+=n;
        if (overfull())
            VTHROW_A(StackAlloc::Overflow);
        return ret;
    }
    void *save_end()
    {
        return end;

        void *saved_end=end;
        return saved_end;
    }
    template <class T>
    T *alloc_end(unsigned n=1) /* throw(StackAlloc::Overflow) */
    {
        end = graehl::align_down((T *)end);
        end = ((T*)end)-1;
        if (overfull())
            VTHROW_A(StackAlloc::Overflow);
        return (T*)end;
    }
    void restore_end(void *saved_end)
    {
        end=saved_end;
    }
    void init(void *begin_, void *end_)
    {
        top=begin_;
        end=end_;
    }
    template <class T>
    unsigned capacity() const
    {
        T *cend=(T*)end;
        T *ctop=graehl::align_up((T*)top);
        if (cend > ctop)
            return cend-ctop;
        else
            return 0;
    }
    /// convenience: allocs and assigns (assumes aligned)
    /// suggested use:  boost::make_function_output_iterator(ref(*this));
    template <class T>
    void operator()(const T& t)
    {
        T*& ttop(*(T**)&top);
        Assert(graehl::is_aligned(ttop));
        if (full())
            VTHROW_A(StackAlloc::Overflow);
        *ttop++=t;
    }
};

# ifdef TEST
#  include "test.hpp"
BOOST_AUTO_TEST_CASE( TEST_STACKALLOC )
{
    const int N=100;
    int a[N];
    StackAlloc s;
    char *top=(char *)a;
    s.init(a,a+N);
    BOOST_CHECK_EQUAL(s.capacity<unsigned>(),N);
    BOOST_CHECK(s.top==a && s.end==a+N);
    s.align<unsigned>();
    BOOST_CHECK(s.top==top);
    BOOST_CHECK(s.alloc<char>()==top && s.top==(top+=1));
    s.align<char>();
    BOOST_CHECK(s.top==top);
    s.align<unsigned>();
    BOOST_CHECK_EQUAL(s.capacity<unsigned>()*sizeof(unsigned),s.capacity<char>());

//    DBP(s.top);

    BOOST_CHECK_EQUAL(s.top,(top=(char *)a+sizeof(unsigned)));
    BOOST_CHECK_EQUAL((void *)s.alloc<unsigned>(2),top);
    BOOST_CHECK_EQUAL(s.top,(top+=2*sizeof(unsigned)));
}

# endif

}

#endif
