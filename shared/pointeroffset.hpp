#ifndef _POINTEROFFSET_HPP
#define _POINTEROFFSET_HPP

#include "myassert.h"
#include "genio.h"

#ifdef TEST
#include "test.hpp"
#endif

template <class T>
inline size_t offset_to_index(const T *a) {
    //return ((unsigned)a)/sizeof(T);
    return (unsigned)(a-(T *)0);
}

template <class T>
inline T * index_to_offset(size_t i) {
//    return (T *)(i*sizeof(T));
    return ((T*)0)+i;
}

// returns difference in bytes rather than #of T.
// forall T *a,T *b: offset_to_index(ptrdiff_offset(a,b)) == a-b
template <class T>
T* offset_ptrdiff(const T *a,const T *b) {
    return (T*)((char *)a - (char *)b);
}

// forall T*a,T*b: ptradd_offset(a,ptrdiff_offset(b,a)) == b
template <class T>
T* offset_ptradd(const T *a, const T *b) {
    return (T*)((char *)a + (ptrdiff_t)b);
}

// equivalent to a+offset_to_index(b)
template <class T, class U>
T* offset_ptradd_rescale(const T *a, const U *b) {
    return (T*)((char *)a + ((ptrdiff_t)b/sizeof(U))*sizeof(T));
}

template <class C>
struct IndexToPointerReader {
    typedef C *value_type;
    C *base;
    IndexToPointerReader(C *_base=0) : base(_base) {}
    template <class charT, class Traits>
    std::basic_istream<charT,Traits>&
    operator()(std::basic_istream<charT,Traits>& in,value_type &l) const {
        ptrdiff_t i;
        in >> i;
        l=base+i;
        return in;
    }
};


/*template <class C>
struct IndexToOffsetReader {
    typedef C *value_type;
    template <class charT, class Traits>
    std::basic_istream<charT,Traits>&
    operator()(std::basic_istream<charT,Traits>& in,value_type &l) const {
        ptrdiff_t i;
        in >> i;
        l=index_to_offset<C>(i);
        return in;
    }
};
*/

template <class C>
struct PointerOffset {
    typedef C* value_type;
    typedef C pointed_type;
    C *offset;
    PointerOffset() : offset(0) {}
    PointerOffset(size_t i) : offset(index_to_offset<C>(i)) {}
    PointerOffset(const PointerOffset &o) : offset(o.offset) {}
    void set_ptrdiff(const C*a,const C*b) {
        offset=offset_ptrdiff(a,b);
    }
    value_type get_offset() const {
        return offset;
    }
    ptrdiff_t get_index() const {
        return offset_to_index(offset);
    }
    value_type add_base(C *base) {
        return offset_ptradd(base,offset);
    }
    void operator =(PointerOffset<C> c) { offset=c.offset; }
    bool operator ==(PointerOffset<C> c) { return offset==c.offset; }
    bool operator <(PointerOffset<C> c) { return offset<c.offset; }
};

template <class charT, class Traits,class C>
std::basic_ostream<charT,Traits>&
operator <<
  (std::basic_ostream<charT,Traits>& os, const PointerOffset<C> &arg)
{
    return os << "index=" << arg.get_index();
}

template <class charT, class Traits,class C>
std::basic_istream<charT,Traits>&
operator >>
  (std::basic_istream<charT,Traits>& in, PointerOffset<C> &arg)
{
    ptrdiff_t i;
    in >> i;
    arg = i;
    return in;
}


#ifdef TEST
BOOST_AUTO_UNIT_TEST( TEST_POINTEROFFSET )
{
}
#endif

#endif
