#ifndef _POINTEROFFSET_HPP
#define _POINTEROFFSET_HPP

#include "myassert.h"
#include "genio.h"
#include "funcs.hpp"
#include <iterator>
#include "debugprint.hpp"

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

// forall T*a,T*b: ptradd_offset(a,ptrdiff_offset(b,a)) == b
template <class T>
T* offset_ptradd(const T *a, size_t b) {
    return a + b;
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
    PointerOffset(size_t i) : offset(index_to_offset<C>(i)) {
//        DBP2(i,get_index());
        Assert(i==get_index());
    }
    PointerOffset(const PointerOffset<C> &o) : offset(o.offset) {}
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
    bool operator !=(PointerOffset<C> c) { return offset!=c.offset; }

//    operator value_type &() { return offset; }
    template <class It>
    struct indirect_iterator : public std::iterator_traits<It> {
        typedef C value_type;
        typedef It iterator_type;
        typedef C *Base;
        typedef indirect_iterator<It> Self;
        It i;
        Base base;
        explicit indirect_iterator(Base _base) : base(_base) {}
        indirect_iterator() {}
/*        template <class C2>
          indirect_iterator(const C2 &_i) : i(_i) {}*/
        indirect_iterator(Base _base, It _i) : i(_i),base(_base) {}
        indirect_iterator(const Self &o) : i(o.i),base(o.base) {}
        Self & operator =(const Self &o) {
            base=o.base;
            i=o.i;
            return *this;
        }
        typedef typename std::iterator_traits<It>::difference_type Diff;
        Self & operator +=(Diff d) {
            i+=d;
            return *this;
        }
        Self & operator -=(Diff d) {
            i-=d;
            return *this;

        }
        Self operator +(Diff d) const {
            Self result(*this);
            return result += d;
        }
        Self operator -(Diff d) const {
            Self result(*this);
            return result -= d;
        }

        Diff operator -(const Self &o) const {
            return i-o.i;
        }

        bool operator <(const Self &o) const { Assert(base==o.base);
            return i < o.i;
        }
        bool operator <=(const Self &o) const { Assert(base==o.base);
            return i <= o.i;
        }
        bool operator >=(const Self &o) const { Assert(base==o.base);
            return i >= o.i;
        }
        bool operator ==(const Self &o) const { Assert(base==o.base);
            return i == o.i;
        }
        bool operator !=(const Self &o) const { Assert(base==o.base);
            return i != o.i;
        }
        bool operator >(const Self &o) const { Assert(base==o.base);
            return i > o.i;
        }

//        operator It & () { return i; }
        Self & operator ++() { ++i; return *this;}
        Self & operator --() { --i; return *this;}
        Self operator --(int) { Self ret=*this;return --ret; }
        Self operator ++(int) { Self ret=*this;return ++ret; }
        value_type & get_value() { return *(i->add_base(base)); }
        ptrdiff_t get_index() { return i->get_index(); }
        value_type & operator *() { return get_value(); }
        value_type * operator ->() { return &get_value(); }
    };
};

template <class T>
struct OffsetBase
{
    typedef T value_type;
    typedef OffsetBase<T> Self;

    T *base;
    OffsetBase(T *base_) : base(base_) {}
    OffsetBase(const Self &o) : base(o.base) {}

    template <class I>
    T& operator [](const I& i) const
    {
        return offset_ptradd(base,i);
    }
};


template <class C>
struct indirect_gt<PointerOffset<C>,C*> {
    typedef PointerOffset<C> I;
    typedef C *B;
    typedef I Index;
    typedef B Base;
    B base;
    indirect_gt(B b) : base(b) {}
    indirect_gt(const indirect_gt<I,B> &o): base(o.base) {}
    bool operator()(I a, I b) const {
        return *a.add_base(base) > *b.add_base(base);
    }
};

template <class C>
struct indirect_lt<PointerOffset<C>,C*> {
    typedef PointerOffset<C> I;
    typedef C *B;
    typedef I Index;
    typedef B Base;
    B base;
    indirect_lt(B b) : base(b) {}
    indirect_lt(const indirect_lt<I,B> &o): base(o.base) {}

    bool operator()(I a, I b) const {
        return a.add_base(base) < b.add_base(base);
    }
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
