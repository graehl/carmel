// if you ever wanted to store a discriminated union of pointer/integer without an extra boolean flag, this will do it, assuming your pointers are never odd.
#ifndef INTORPOINTER_HPP
#define INTORPOINTER_HPP

#include "myassert.h"
#include "genio.h"

#ifdef TEST
#include "test.hpp"
#endif

#include "pointeroffset.hpp"

template <class Pointed=void,class Int=size_t>
struct IntOrPointer {
    typedef Pointed pointed_type;
    typedef Int integer_type;
    typedef Pointed *value_type;
    typedef IntOrPointer<Pointed> Self;
    IntOrPointer(int j) { *this=j; }
    IntOrPointer(size_t j) { *this=j; }
    IntOrPointer(value_type v) { *this=v; }
    union {
        value_type p; // must be even (guaranteed unless you're pointing at packed chars)
        integer_type i; // stored as 2*data+1, so only has half the range (one less bit) of a normal integer_type
    };
    bool is_integer() const { return i & 1; }
    bool is_pointer() const { return !is_integer(); }
    value_type & pointer() { return p; }
    const value_type & pointer() const { Assert(is_pointer()); return p; }
    integer_type integer() const { Assert(is_integer()); return i >> 1; }
    /// if sizeof(C) is even, could subtract sizeof(C)/2 bytes from base and add directly i * sizeof(C)/2 bytes
    template <class C>
    C* offset_integer(C *base) {
        return base + integer();
    }
    template <class C>
    C* offset_pointer(C *base) {
        //return C + offset_to_index(pointer());
        return offset_ptradd_rescale(base,pointer());
    }
    void operator=(unsigned j) { i = 2*(integer_type)j+1; }
    void operator=(int j) { i = 2*(integer_type)j+1; }
    template <class C>
    void operator=(C j) { i = 2*(integer_type)j+1; }
    void operator=(value_type v) { p=v; }
    IntOrPointer() {}
    IntOrPointer(const Self &s) : p(s.p) {}
    void operator=(const Self &s) { p=s.p; }
    template <class C>
    bool operator ==(C* v) { return p==v; }
    template <class C>
    bool operator ==(const C* v) { return p==v; }
    template <class C>
    bool operator ==(C j) { return integer() == j; }
    bool operator ==(Self s) { return p==s.p; }
    bool operator !=(Self s) { return p!=s.p; }
    GENIO_print {
        if (is_integer())
            o << integer();
        else {
            o << "0x" << std::hex << (size_t)pointer() << std::dec;
        }
        return GENIOGOOD;
    }
};

template <class charT, class Traits,class Pointed,class Int>
std::basic_ostream<charT,Traits>&
operator <<
    (std::basic_ostream<charT,Traits>& os, const IntOrPointer<Pointed,Int> &arg)
{
  return gen_inserter(os,arg);
}



#ifdef TEST
BOOST_AUTO_UNIT_TEST( TEST_INTORPOINTER )
{
    int i=3,k;
    IntOrPointer<int> p(5);
    IntOrPointer<int> v(&i);
    BOOST_CHECK(p.is_integer());
    BOOST_CHECK(!p.is_pointer());
    BOOST_CHECK(p == 5);
    p=0;
    BOOST_CHECK(p.is_integer());
    BOOST_CHECK(p.integer() == 0);
    BOOST_CHECK(p == 0);
    BOOST_CHECK(p!=v);

    p=&i;
    BOOST_CHECK(p.is_pointer());
    BOOST_CHECK(p.pointer() == &i);
    BOOST_CHECK(p == &i);
    BOOST_CHECK(p==v);

    p=index_to_offset<int>(3);
    BOOST_CHECK(offset_to_index(p.pointer()) == 3);
    BOOST_CHECK(&k+3 == offset_ptradd(&k,p.pointer()));
    BOOST_CHECK(p.offset_pointer(&k) == &k+3);
    wchar_t j;
    BOOST_CHECK(p.offset_pointer(&j) == &j+3);
}
#endif

#endif
