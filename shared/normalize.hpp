#ifndef NORMALIZE_HPP
#define NORMALIZE_HPP

#include "dynarray.h"

#include "pointeroffset.hpp"

#include "container.hpp"
#include "byref.hpp"
#include "genio.h"
#include "threadlocal.hpp"
#include "funcs.hpp"
#include <algorithm>
#include "debugprint.hpp"
#include "swapbatch.hpp"


//FIXME: leave rules that don't occur in normalization groups alone (use some original/default value)
template <class W>
struct NormalizeGroups {
    typedef W weight_type;
    typedef PointerOffset<W> value_type; // pointer offsets
    typedef Array<value_type> Group;
    typedef SwapBatch<Group> Groups;
    max_in_accum<value_type> max_offset;
    size_accum<size_t> total_size;

    NormalizeGroups(std::string basename,unsigned batchsize)  : norm_groups(basename,batchsize)
    {
        //,value_type watch_value
//        if (watch_value.get_offset())

    }

    Groups norm_groups;

    template <class charT, class Traits>
    void
    get_from(std::basic_istream<charT,Traits>& in)
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

    typename Groups::iterator find_group_holding(value_type v) {
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

    size_t max_index() const {
        return max_offset.max.get_index();
    }
    size_t required_size() const {
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
        out << ng << " normalization groups, "  << npar<<" parameters, "<<(float)npar/ng<<" average parameters/group, "<<max_params()<< " max.";
    }
    void operator ()(Group &i) {
        GIt end=i.end(), beg=i.begin();
        weight_type sum=0;
        for (GIt j=beg;j!=end;++j) {
            weight_type &w=*(j->add_base(base));
            sum+=w;
        }
#define DODIFF(d,w) do {weight_type diff = absdiff(d,w);if (maxdiff<diff) {maxdiff_index=j->get_index();DBP5(d,w,maxdiff,diff,maxdiff_index);maxdiff=diff;} } while(0)
        if (sum > 0) {
            DBPC2("Normalization group with",sum);
            for (GIt j=beg;j!=end;++j) {
                weight_type &w=*(j->add_base(base));
                weight_type &d=*(j->add_base(dest));
                DBP4(j->get_index(),d,w,w/sum);
                weight_type prev=d;
                d=w/sum;
                DODIFF(d,prev);
            }
        } else {
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
        init(w,set_random_pos_fraction());
    }

// array must have values for all max_index()+1 rules
// returns maximum change
    void normalize(W *array_base) {
        normalize(array_base,array_base);
    }
    void normalize(W *array_base, W* _dest, int _zerocounts=UNIFORM_ZEROCOUNTS, ostream *_log=NULL) {
        base=array_base;
        dest=_dest;
        maxdiff.setZero();
//        DBP(maxdiff);
        DBP_INC_VERBOSE;
#ifdef DEBUG
        unsigned size=required_size();
#endif
        DBPC2("Before normalize from base->dest",Array<W>(base,base+size));

        zerocounts=_zerocounts;
        log=_log;
        enumerate(norm_groups,ref(*this));
        DBPC2("After normalize:",Array<W>(dest,dest+size));
    }
    GENIO_print_on
    {
        return norm_groups.print_on(o);
    }

};

template <class charT, class Traits,class C>
std::basic_istream<charT,Traits>&
operator >>
(std::basic_istream<charT,Traits>& is, NormalizeGroups<C> &arg)
{
    arg.get_from(is);
    return is;
}


template <class charT, class Traits,class C>
std::basic_ostream<charT,Traits>&
operator <<
    (std::basic_ostream<charT,Traits>& o, const NormalizeGroups<C> &arg)
{
//    return os << arg.norm_groups;
/*    os << "(\n";
    arg.norm_groups.enumerate(LineWriter());
    os << ")\n";
*/
    arg.print_on(o);
    return o;

}

#ifdef TEST
#include "test.hpp"
#endif

#ifdef TEST
BOOST_AUTO_UNIT_TEST( TEST_NORMALIZE )
{
    typedef Weight W;
    FixedArray<W> w(10u);
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

#endif
