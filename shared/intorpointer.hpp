#ifndef _INTORPOINTER_HPP
#define _INTORPOINTER_HPP

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
    typedef void has_print_on;
    GENIO_print_on {
        if (is_integer())
            o << integer();
        else {
            o << "0x" << hex << (size_t)pointer() << dec;
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
    int i=3;
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
}
#endif

#endif
