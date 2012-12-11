// normalization to 1 for each group in a disjoint partition of indices - on-disk normalization groups (memmappable) for large scale
#ifndef GRAEHL_SHARED__NORMALIZE_HPP
#define GRAEHL_SHARED__NORMALIZE_HPP

#include <graehl/shared/dynarray.h>
#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#endif


//#include "pointeroffset.hpp"
/*
  template <class C>
struct indirect_gt<size_t,C*> {
    typedef size_t I;
    typedef C *B;
    typedef I Index;
    typedef B Base;
    B base;
    indirect_gt(B b) : base(b) {}
    indirect_gt(const indirect_gt<I,B> &o): base(o.base) {}
    bool operator()(I a, I b) const {
        return base[a] > base[b];
    }
};
// HANDLED IN funcs.hpp
*/
#include <graehl/shared/container.hpp>
#include <graehl/shared/byref.hpp>
#include <graehl/shared/genio.h>
#include <graehl/shared/threadlocal.hpp>
#include <graehl/shared/funcs.hpp>
#include <algorithm>
#include <graehl/shared/debugprint.hpp>
#include <graehl/shared/swapbatch.hpp>
#include <graehl/shared/statistics.hpp>

namespace graehl {

//FIXME: leave rules that don't occur in normalization groups alone (use some original/default value)
template <class Wsource,class Wdest=Wsource>
struct NormalizeGroups {
    typedef NormalizeGroups<Wsource,Wdest> self_type;
    typedef Wsource source_t;
    typedef Wdest dest_t;
//    typedef PointerOffset<W> index_type; // pointer offsets
    typedef size_t index_type;
    typedef index_type offset_type;
    typedef array<offset_type> Group;
    typedef SwapBatch<Group> Groups;
    max_in_accum<offset_type> max_offset;
    size_accum<size_t> total_size;
    Groups norm_groups;

    NormalizeGroups(std::string basename,unsigned batchsize,source_t add_k_smoothing_=0)  : norm_groups(basename,batchsize), add_k_smoothing(add_k_smoothing_)
    {
        //,index_type watch_value
//        if (watch_value.get_offset())

    }


    template <class charT, class Traits>
    void
    read(std::basic_istream<charT,Traits>& in)
    {
        char c;
        EXPECTCH_SPACE('('); //FIXME: rationalize w/ dynarray input w/ optional '('->eof?  not possible?
        norm_groups.read_all_enumerate(in,make_both_functors_byref(max_offset,total_size),')');
    }

    unsigned num_groups() const {
        return norm_groups.size();
    }
    //FIXME: accumulate on read
    size_t num_params()  const {
        return total_size;
    }
    size_t max_params() const
    {
        return total_size.maximum();
    }

    typename Groups::iterator find_group_holding(offset_type v) {
        typename Groups::iterator i=norm_groups.begin(),e=norm_groups.end();
        DBPC3("find group",v,norm_groups);
        for (;i!=e;++i) {
            DBP_ADD_VERBOSE(2);
            DBP2(*i,v);
            Group &gi=*i;
            typename Group::iterator e2=gi.end();
            if (std::find(gi.begin(),e2,v) != e2)
                return i;
        }
        return e;
    }
    static size_t get_index(offset_type i) {
        return i;
    }
    size_t max_index() const {
        return get_index(max_offset);
    }
    size_t required_size() const {
        return max_index()+1;
    }

    source_t *base;
    dest_t *dest;
    dest_t maxdiff;
    source_t add_k_smoothing;
    std::ostream *log;
    enum make_not_anon_18 { ZERO_ZEROCOUNTS=0,SKIP_ZEROCOUNTS=1,UNIFORM_ZEROCOUNTS=2};
    int zerocounts; // use enum vals
    size_t maxdiff_index;
    typedef typename Group::iterator GIt;
    typedef typename Group::const_iterator GItc;
    void print_stats(std::ostream &out=std::cerr) const {
        unsigned npar=num_params();
        unsigned ng=num_groups();
        out << ng << " normalization groups, "  << npar<<" parameters, "<<(float)npar/ng<<" average parameters/group, "<<max_params()<< " max.";
    }
    source_t &source(offset_type index) const {
        return base[index];
    }
    dest_t &sink(offset_type index) const {
        return dest[index];
    }
    void operator ()(Group &i) {
        GIt end=i.end(), beg=i.begin();
        source_t sum=0;
        for (GIt j=beg;j!=end;++j) {
            source_t &w=source(*j);
            sum+=w;
        }
#define DODIFF(d,w) do {dest_t diff = absdiff(d,w);if (maxdiff<diff) {maxdiff_index=get_index(*j);DBP5(d,w,maxdiff,diff,maxdiff_index);maxdiff=diff;} } while(0)
        if (sum > 0) {
            sum+=add_k_smoothing; // add to denominator
            DBPC2("Normalization group with",sum);
            for (GIt j=beg;j!=end;++j) {
                source_t &w=source(*j);
                dest_t &d=sink(*j);
                DBP4(get_index(*j),d,w,w/sum);
                dest_t prev=d;
                d=w/sum;
                DODIFF(d,prev);
            }
        } else {
            if(log)
                *log << "Zero counts for normalization group #" << 1+(&i-norm_groups.begin())  << " with first parameter " << get_index(*beg) << " (one of " << i.size() << " parameters)";
            if (zerocounts!=SKIP_ZEROCOUNTS) {
                dest_t setto;
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
                    dest_t &d=sink(*j);
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
    template <class V> // v(normindex,paramid,normsize) where paramid is in normindex.  if end_index>0, v(i) for any unseen i<end_index
    void visit_norm_param(V &v,index_type end_index=0) {
        fixed_array<bool> seen(end_index);
        unsigned normi=0;
        for (Groups::iterator g=norm_groups.begin(),ge=norm_groups.end();g!=ge;++g) {
            ++normi;
            Group const& group=*g;
            unsigned gsz=group.size();
            for (GItc p=group.begin(),pe=group.end();p!=pe;++p) {
                index_type i=*p;
                v(normi,i,gsz);
                if (i<end_index) seen[i]=true;
            }
        }
        for (index_type i=0;i<end_index;++i)
            if (!seen[i])
                v(i);
    }

    template <class T> // enumerate:
    void visit(Group &group, T tag) {
        GIt beg=group.begin(),end=group.end();
        dest_t sum=0;
        for (GIt i=beg;i!=end;++i) {
            dest_t &w=sink(*i);
            tag(w);
            sum += w;
        }
        if (sum > 0)
            for (GIt i=beg;i!=end;++i) {
                dest_t &w=sink(*i);
                w /= sum;
            }
    }
    template <class T>
    void init(dest_t *w,T tag) {
        dest=w;
        enumerate(norm_groups,*this,tag); // calls visit(group,tag)
    }
    void init_uniform(dest_t *w) {
        init(w,set_one());
    }
    void init_random(dest_t *w) {
        base=w;
        init(w,set_random_pos_fraction());
    }

// array must have values for all max_index()+1 rules
    //FIXME: only works if source_t = dest_t
    void normalize(source_t *array_base) {
        normalize(array_base,array_base);
    }
    void normalize(source_t *array_base, dest_t* _dest, int _zerocounts=UNIFORM_ZEROCOUNTS, std::ostream *_log=NULL) {
        base=array_base;
        dest=_dest;
        maxdiff.setZero();
//        DBP(maxdiff);
        DBP_INC_VERBOSE;
#ifdef DEBUG
        unsigned size=required_size();
#endif
        DBPC2("Before normalize from base->dest",array<source_t>(base,base+size));

        zerocounts=_zerocounts;
        log=_log;
        enumerate(norm_groups,boost::ref(*this));
        DBPC2("After normalize:",array<dest_t>(dest,dest+size));
    }
    template <class O> void print(O&o) const
    {
        norm_groups.print(o);
    }
    TO_OSTREAM_PRINT
    FROM_ISTREAM_READ
};

/*
template <class charT, class Traits,class W1,class W2>
std::basic_istream<charT,Traits>&
operator >>
(std::basic_istream<charT,Traits>& is, NormalizeGroups<W1,W2> &arg)
{
    arg.read(is);
    return is;
}


template <class charT, class Traits,class W1,class W2>
std::basic_ostream<charT,Traits>&
operator <<
              (std::basic_ostream<charT,Traits>& o, const NormalizeGroups<W1,W2> &arg)
{
    arg.print(o);
    return o;
}
*/

#ifdef GRAEHL_TEST
BOOST_AUTO_TEST_CASE( TEST_NORMALIZE )
{
    using namespace std;
    using namespace graehl;
    typedef Weight W;
    fixed_array<W> w(10u);
    w[0]=1;
    w[1]=2;
    w[2]=3;
    w[3]=4;
    w[4]=1;
    w[5]=2;
    w[6]=3;
    w[7]=4;
    w[8]=1;
    w[9]=2;

    NormalizeGroups<W> ng("tmp.test.normalize",32);
    string s="((0 1) (2 3) (4 5 6) (7 8 9))";
    istringstream is(s);
    BOOST_CHECK(is >> ng);
    BOOST_CHECK(ng.max_index() == 9);
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

}

#endif
