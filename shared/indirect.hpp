#ifndef INDIRECT_HPP
#define INDIRECT_HPP

#include <algorithm>

#define INDIRECT_STRUCT_COMMON(type,method) \
    struct indirect_ ## type ## _ ## method                  \
    {                                         \
        typedef type indirect_type;           \
        indirect_ ## type ## _ ## method *pimp;              \
        indirect_ ## type ## _ ## method(type &t) : pimp(&t) {} \
        indirect_ ## type ## _ ## method(const indirect_ ## type ## _ ## method &t) : pimp(t.pimp) {} \

#define INDIRECT_STRUCT(type,method)                         \
    INDIRECT_STRUCT_COMMON(type,method) \
            void operator()() const         \
            {                                   \
                pimp->method();               \
            }                                   \
    };

#define INDIRECT_STRUCT1(type,method,t1,v1)      \
    INDIRECT_STRUCT_COMMON(type,method) \
            void operator()(t1 v1) const         \
            {                                   \
                pimp->method(v1);               \
            }                                   \
    };


#define INDIRECT_STRUCT1(type,method,t1,v1)      \
    INDIRECT_STRUCT_COMMON(type,method) \
            void operator()(t1 v1) const         \
            {                                   \
                pimp->method(v1);               \
            }                                   \
    };



#define INDIRECT_PROTO(type,method) INDIRECT_STRUCT(type,method)    \
        void method()

#define INDIRECT_PROTO1(type,method,t1,v1) INDIRECT_STRUCT1(type,method,t1,v1)    \
        void method(t1 v1)

#define INDIRECT_PROTO2(type,method,t1,v1,t2,v2) INDIRECT_STRUCT2(type,method,t2,v2)   \
        void method(t1 v1,t2 v2)

namespace graehl {

template <class T,class Comp>
struct compare_indirect_array : public Comp
{
    const T *base;
    typedef compare_indirect_array<T,Comp> Self;
    compare_indirect_array(T *base_=NULL,const Comp &comp=Comp()) : base(base_),Comp(comp) {}
    compare_indirect_array(const Self &s) : base(s.base),Comp(s) {}
    template <class Vec,class VecTo>
    compare_indirect_array(const Vec &from,VecTo &to,const Comp &comp=Comp()) : Comp(comp) 
    {
        init(from,to);
    }
    template <class Vec,class VecTo>
    void init(const Vec &from,VecTo &to)
    {
        base=&*from.begin();
        to.clear();
        for (unsigned i=0,e=from.size();i<e;++i)
            to.push_back(i);
    }    
    bool operator()(unsigned a,unsigned b) const
    {
        return Comp::operator()(base[a],base[b]);
    }
};


template <class I, class B,class C>
struct indirect_cmp : public C {
    typedef C Comp;
    typedef I Index;
    typedef B Base;
    typedef indirect_cmp<I,B,C> Self;

    B base;

    indirect_cmp(const B &b,const Comp&comp=Comp()) : base(b), Comp(comp) {}
    indirect_cmp(const Self &o): base(o.base), Comp((Comp &)o) {}

    bool operator()(const I &a, const I &b) const {
        return Comp::operator()(base[a], base[b]);
    }
};

template <class I,class B,class C>
void top_n(unsigned n,I begin,I end,const B &base,const C& comp)
{
    indirect_cmp<I,B,C> icmp(base,comp);
    I mid=begin+n;
    if (mid >= end)
        std::sort(begin,end,icmp);
    else
        std::partial_sort(begin,mid,end,icmp);
}

// useful for sorting; could parameterize on predicate instead of just <=lt, >=gt
template <class I, class B>
struct indirect_lt {
    typedef I Index;
    typedef B Base;
    B base;
    indirect_lt(const B &b) : base(b) {}
    indirect_lt(const indirect_lt<I,B> &o): base(o.base) {}

    bool operator()(const I &a, const I &b) const {
        return base[a] < base[b];
    }
};

template <class I, class B>
struct indirect_gt {
    typedef I Index;
    typedef B Base;
    B base;
    indirect_gt(const B &b) : base(b) {}
    indirect_gt(const indirect_gt<I,B> &o): base(o.base) {}
    bool operator()(const I &a, const I &b) const {
        return base[a] > base[b];
    }
};

}


#endif
