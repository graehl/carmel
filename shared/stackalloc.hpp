#ifndef _STACKALLOC_HPP
#define _STACKALLOC_HPP

#include <stdexcept>
#include "funcs.hpp"

struct StackAlloc
{
    // throws when allocation fails (I don't let you check ahead of time)
#if 0
    struct Overflow //: public std::exception
    {
    };
#else
    typedef std::exception Overflow;
#endif

    void *top;
    void *end;
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
        top=::align((T*)top);
    }
    template <class T>
    T* aligned_alloc(unsigned n=1) throw(StackAlloc::Overflow)
    {
        this->align<T>();
        return alloc<T>(n);
    }
    bool overfull() const
    {
        return top > end;  // important: different size T will be allocated, and
                         // no guarantee that all T divide space equally (even
                         // for alloc(1)
    }
    template <class T>
    T* alloc(unsigned n=1) throw(StackAlloc::Overflow)
    {
        T*& ttop(*(T**)&top);
//        Assert(is_aligned(ttop));
        T* ret=ttop;
        ttop+=n;
        if (overfull())
            throw StackAlloc::Overflow();
        return ret;
    }
    void *save_end()
    {
        return end;

        void *saved_end=end;
        return saved_end;
    }
    template <class T>
    T *alloc_end(unsigned n=1) throw(StackAlloc::Overflow)
    {
        end = ::align_down((T *)end);
        end = ((T*)end)-1;
        if (overfull())
            throw StackAlloc::Overflow();
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
        T *ctop=::align((T*)top);
        if (cend > ctop)
            return cend-ctop;
        else
            return 0;
    }
};

# ifdef TEST
#  include "test.hpp"
BOOST_AUTO_UNIT_TEST( TEST_STACKALLOC )
{
#  define N 100
    int a[N];
    StackAlloc s;
    char *top=(char *)a;
    s.init(a,a+N);
    CHECK_EQ(s.capacity<unsigned>(),N);
    CHECK(s.top==a && s.end==a+N);
    s.align<unsigned>();
    CHECK(s.top==top);
    CHECK(s.alloc<char>()==top && s.top==(top+=1));
    s.align<char>();
    CHECK(s.top==top);
    s.align<unsigned>();
    CHECK_EQ(s.capacity<unsigned>(),s.capacity<char>()*sizeof(unsigned));

    DBP(s.top);

    CHECK_EQ(s.top,(top=(char *)a+sizeof(unsigned)));
    CHECK_EQ((void *)s.alloc<unsigned>(2),top);
    CHECK_EQ(s.top,(top+=2*sizeof(unsigned)));
}

# endif


#endif
