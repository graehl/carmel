#ifndef _NORMALIZE_HPP
#define _NORMALIZE_HPP

#include "dynarray.h"

#include "pointeroffset.hpp"

#include "container.hpp"
#include "byref.hpp"
#include "genio.h"
#include "threadlocal.hpp"
#include "functors.hpp"
#include <algorithm>
#include "debugprint.hpp"

//FIXME: leave rules that don't occur in normalization groups alone (use some original/default value)
template <class W>
struct NormalizeGroups {
    typedef W weight_type;
    typedef PointerOffset<W> value_type; // pointer offsets
    typedef FixedArray<value_type> Group;
    typedef FixedArray<Group> Groups;
    Groups norm_groups;
//    value_type max_offset;
    #ifdef NORMALIZE_SEEN
    DynamicArray<bool> seen_index;
    #endif
    unsigned num_groups() const {
        return norm_groups.size();
    }
    unsigned num_params() const {
        unsigned sum=0;
        for (typename Groups::const_iterator i=norm_groups.begin(),e=norm_groups.end();i!=e;++i)
            sum+=i->size();
        return sum;
    }
    Group *find_group_holding(value_type v) {
        for (typename Groups::iterator i=norm_groups.begin(),e=norm_groups.end();i!=e;++i)
            if (std::find(i->begin(),i->end(),v))
                return &(*i);
        return NULL;
    }
//    GENIO_get_from {
//        IndirectReader<IndexToOffsetReader<W> > reader;
//        norm_groups.get_from(in,reader);
//    }

#ifdef NORMALIZE_SEEN
    struct seen_offset_p {
        DynamicArray<bool> *seen_index;
        void operator()(value_type v) {
            (*seen_index)(v.get_index())=true; // default init of false when expanded
        }
        seen_offset_p(DynamicArray<bool> *a) : seen_index(a) {}
        seen_offset_p(const seen_offset_p &o) : seen_index(o.seen_index) {}
    };
#endif
    struct max_p {
        value_type max; // default init = 0
        void operator()(value_type v) {
            if (max < v)
                max = v;
        }
    };
    size_t max_index() {
#ifdef NORMALIZE_SEEN
        nested_enumerate(norm_groups,seen_offset_p(&seen_index));
        return seen_index.size();
#else
        max_p m;
        nested_enumerate(norm_groups,ref(m));
        return m.max.get_index();
#endif
    }
    size_t required_size() {
        return max_index()+1;
    }

    W *base;
    W *dest;
    W maxdiff;
    std::ostream *log;
    enum { ZERO_ZEROCOUNTS=0,SKIP_ZEROCOUNTS=1,UNIFORM_ZEROCOUNTS=2};
    int zerocounts; // use enum vals
    unsigned maxdiff_index;
    typedef typename Group::iterator GIt;
    void print_stats(std::ostream &out=std::cerr) const {
        unsigned npar=num_params();
        unsigned ng=num_groups();
        out << ng << " normalization groups, "  << npar<<" parameters, "<<(float)npar/ng<<" average parameters/group";
    }
    void operator ()(Group &i) {
        GIt end=i.end(), beg=i.begin();
        weight_type sum=0;
        for (GIt j=beg;j!=end;++j) {
            weight_type &w=*(j->add_base(base));
            sum+=w;
        }
#define DODIFF(d,w) do {weight_type diff = absdiff(d,w);if (maxdiff<diff) {maxdiff_index=j->get_index();DBP5(d,w,maxdiff,diff,maxdiff_index);maxdiff=diff;} } while(0)
        if (sum > 0)
            for (GIt j=beg;j!=end;++j) {
                weight_type &w=*(j->add_base(base));
                weight_type &d=*(j->add_base(dest));
                weight_type prev=d;
                d=w/sum;
                DODIFF(d,prev);
            }
        else {
            if(log)
                *log << "Zero counts for normalization group #" << 1+(&i-norm_groups.begin())  << " with first parameter " << beg->get_index() << " (one of " << i.size() << " parameters)";
            if (zerocounts!=SKIP_ZEROCOUNTS) {
                weight_type setto;
                if (zerocounts == UNIFORM_ZEROCOUNTS) {
                    setto=1. / (end-beg);
                    if(log)
                        *log << " - setting to uniform probability " << setto << std::endl;
                } else {
                    setto=0;
                    if(log)
                        *log << " - setting to zero probability." << std::endl;
                }
                for (GIt j=beg;j!=end;++j) {
                    weight_type &w=*(j->add_base(base));
                    weight_type &d=*(j->add_base(dest));
                    DODIFF(d,setto);
                    d=setto;
                }
            }
        }
#undef DO_DIFF
    }
#ifdef NORMALIZE_SEEN
    void copy_unseen(W *src, W *to) {
        for (unsigned i=0,e=seen_index.size();i!=e;++i) {
            if (!seen_index[i])
                to[i]=src[i];
        }
    }
    void set_unseen_to(W src, W *to) {
        for (unsigned i=0,e=seen_index.size();i!=e;++i) {
            if (!seen_index[i])
                to[i]=src;
        }
    }
    template <class F>
    void enumerate_seen(F f) {
        for (unsigned i=0,e=seen_index.size();i!=e;++i) {
            if (seen_index[i])
                f(i);
        }
    }
    template <class F>
    void enumerate_unseen(F f) {
        for (unsigned i=0,e=seen_index.size();i!=e;++i) {
            if (!seen_index[i])
                f(i);
        }
    }
#endif
    template <class T>
    void visit(Group &group, T tag) {
        GIt beg=group.begin(),end=group.end();
        W sum=0;
        for (GIt i=beg;i!=end;++i) {
            W &w=*(i->add_base(base));
            tag(w);
            sum += w;
        }
        if (sum > 0)
            for (GIt i=beg;i!=end;++i) {
                W &w=*(i->add_base(base));
                w /= sum;
            }
    }
    template <class T>
    void init(W *w,T tag) {
        base=w;
        enumerate(norm_groups,*this,tag);
    }
    void init_uniform(W *w) {
        init(w,set_one());
    }
    void init_random(W *w) {
        base=w;
        init(w,set_rand_pos_fraction());
    }

// array must have values for all max_index()+1 rules
// returns maximum change
    void normalize(W *array_base) {
        normalize(array_base,array_base);
    }
    void normalize(W *array_base, W* _dest, int _zerocounts=UNIFORM_ZEROCOUNTS, ostream *_log=NULL) {
//        SetLocal<W*> g1(base,array_base);
//       SetLocal<W*> g2(dest,_dest);
        base=array_base;
        dest=_dest;
        maxdiff.setZero();
        DBP(maxdiff);
        zerocounts=_zerocounts;
        log=_log;
        enumerate(norm_groups,ref(*this));
    }
};

template <class charT, class Traits,class C>
std::basic_istream<charT,Traits>&
operator >>
(std::basic_istream<charT,Traits>& is, NormalizeGroups<C> &arg)
{
    return is >> arg.norm_groups;
//    return gen_extractor(is,arg);
}


template <class charT, class Traits,class C>
std::basic_ostream<charT,Traits>&
operator <<
    (std::basic_ostream<charT,Traits>& os, const NormalizeGroups<C> &arg)
{
    return os << arg.norm_groups;
}

#ifdef TEST
#include "test.hpp"
#endif

#ifdef TEST
BOOST_AUTO_UNIT_TEST( TEST_NORMALIZE )
{
    typedef Weight W;
    FixedArray<W> w(4u);
    w[0]=1;
    w[1]=2;
    w[2]=3;
    w[3]=4;
    NormalizeGroups<W> ng;
    string s="((1) (2 3))";
    istringstream is(s);
    BOOST_CHECK(is >> ng);
    BOOST_CHECK(ng.max_index() == 3);
//    cerr << Weight::out_always_real;
//    cout << Weight::out_variable;
//    DBP(w);
//    DBP(ng);
    ng.normalize(w.begin());
//    BOOST_CHECK_CLOSE(w[2].getReal()+w[3].getReal(),1,1e-6);
//    BOOST_CHECK_CLOSE(w[2].getReal()*4,w[3].getReal()*3,1e-6);

//    DBP(w);
}
#endif

#endif
