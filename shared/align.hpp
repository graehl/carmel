#ifndef ALIGN_HPP_inc
#define ALIGN_HPP_inc

#include <boost/type_traits/alignment_traits.hpp>
#include <graehl/shared/myassert.h>

#ifdef TEST
#include <graehl/shared/test.hpp>
#endif

namespace graehl {

template <class T>
T *align_up(T *p)
{
    const unsigned align=boost::alignment_of<T>::value;
    const unsigned align_mask=(align-1);

    Assert2((align_mask & align), ==0); // only works for power-of-2 alignments.
    char *cp=(char *)p;
    cp += align-1;
    cp -= (align_mask & (unsigned)cp);
//    DBP4(sizeof(T),align,(void*)p,(void*)cp);
    return (T*)cp;
}

template <class T>
T *align_down(T *p)
{
    const unsigned align=boost::alignment_of<T>::value;
    const unsigned align_mask=(align-1);
//    DBP3(align,align_mask,align_mask & align);
    //      unsigned & ttop(*(unsigned *)&p); //FIXME: do we need to warn compiler about aliasing here?
//            ttop &= ~align_mask;
    //return p;
    Assert2((align_mask & align), == 0); // only works for power-of-2 alignments.
    unsigned diff=align_mask & (unsigned)p; //= align-(ttop&align_mask)
    return (T*)((char *)p - diff);
}

template <class T>
bool is_aligned(T *p)
{
    //      unsigned & ttop(*(unsigned *)&p); //FIXME: do we need to warn compiler about aliasing here?
    const unsigned align=boost::alignment_of<T>::value;
    const unsigned align_mask=(align-1);
    return !(align_mask & (unsigned)p);
}


#ifdef TEST
#include <graehl/shared/test.hpp>
BOOST_AUTO_UNIT_TEST( TEST_ALIGN )
{
    unsigned *p;
    p=(unsigned *)0x15;
    BOOST_CHECK_EQUAL(::align_up(p),(unsigned *)0x18);
    BOOST_CHECK_EQUAL(::align_down(p),(unsigned *)0x14);
    BOOST_CHECK(!is_aligned(p));

    p=(unsigned *)0x16;
    BOOST_CHECK_EQUAL(::align_up(p),(unsigned *)0x18);
    BOOST_CHECK_EQUAL(::align_down(p),(unsigned *)0x14);
    BOOST_CHECK(!is_aligned(p));

    p=(unsigned *)0x17;
    BOOST_CHECK_EQUAL(::align_up(p),(unsigned *)0x18);
    BOOST_CHECK_EQUAL(::align_down(p),(unsigned *)0x14);
    BOOST_CHECK(!is_aligned(p));

    p=(unsigned *)0x28;
    BOOST_CHECK_EQUAL(::align_up(p),p);
    BOOST_CHECK_EQUAL(::align_down(p),p);
    BOOST_CHECK(is_aligned(p));

    const unsigned N=200;
    const unsigned J=sizeof(int);
    for (unsigned i=0;i<N;++i) {
        unsigned word=i/J;
        unsigned wordup=((i%J) ? word+1 : word);
        int *p0=0;
        BOOST_CHECK_EQUAL(align_up((int *)i),p0+wordup);
        BOOST_CHECK_EQUAL(align_down((int *)i),p0+word);
        BOOST_CHECK_EQUAL((void*)align_up((char*)i),(void*)(char*)i);
        BOOST_CHECK_EQUAL((void*)align_down((char*)i),(void*)(char*)i);        
    }

}

#endif
}

#endif
