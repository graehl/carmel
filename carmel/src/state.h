#ifndef GRAEHL_CARMEL__STATE_H
#define GRAEHL_CARMEL__STATE_H

#include <graehl/shared/config.h>
#include <graehl/shared/dynarray.h>
#include <graehl/shared/myassert.h>
#include <graehl/shared/2hash.h>
#include <graehl/shared/weight.h>
#include <graehl/shared/list.h>
#include <graehl/shared/arc.h>
#include <iostream>


namespace graehl {

struct IntKey {
    int i;
    size_t hash() const { return uint32_hash(i); }
    IntKey() {}
    IntKey(int a) : i(a) {}
    operator int() const { return i; }
};

struct IOPair {
    int in;
    int out;
    size_t hash() const
    {
        return uint32_hash(1543 * out + in);
    }
    IOPair() {}
    IOPair(int in,int out) : in(in),out(out) {}
};

std::ostream & operator << (std::ostream &o, IOPair p);

inline bool operator == (const IOPair l, const IOPair r) {
    return l.in == r.in && l.out == r.out;
}

}//ns

BEGIN_HASH_VAL(graehl::IntKey) {	return x.hash(); } END_HASH
BEGIN_HASH_VAL(graehl::IOPair) {	return x.hash(); } END_HASH

namespace graehl {

struct State {
    /*
    BOOST_STATIC_CONSTANT(int,input=0);
    BOOST_STATIC_CONSTANT(int,output=1);
    */
    enum {input=0,output=1,none=2};

    typedef List<FSTArc> Arcs;

    // note: loses tie groups.
    // openfst.org MutableFst<LogArc>, (or StdArc) eg StdVectorFst<StdArc>
    template <class Fst>
    void to_openfst(Fst &fst,unsigned source) const
    {
        typedef typename Fst::Arc A;
        typedef typename A::Weight W;
        typedef typename A::StateId I;
        for ( Arcs::const_iterator a=arcs.begin(),end=arcs.end() ; a != end ; ++a ) {
            fst.AddArc(source,A(a->in,a->out,W(a->weight.getNegLn()),a->dest));
            // note: just like we do, openfst uses index 0 for epsilons
        }
    }

    template <class V>
    void visit_arcs(V &v) const
    {
        for (Arcs::const_iterator i=arcs.begin(),e=arcs.end();i!=e;++i) {
            v(*i);
        }
    }

    template <class V>
    void visit_arcs(unsigned s,V &v)
    {
        for (Arcs::val_iterator i=arcs.val_begin(),e=arcs.val_end();i!=e;++i) {
            v(s,*i);
        }
    }

    // respects locked/tied arcs.  f(&weight) changes weight once per group or ungrouped arc, and never for locked
    template <class F> struct modify_parameter_once : public F
    {
        typedef FSTArc::group_t G;
        typedef HashTable<G, Weight> HT;
        HT tied;
        modify_parameter_once(F const& f) : F(f) {}
        void operator()(unsigned source,FSTArc &a)
        {
            if (a.isLocked()) return;
            if (a.isNormal())
                F::operator()(&a.weight);
            else { //tied
                HT::insert_return_type it=tied.insert(HT::value_type(a.groupId,0));
                if (it.second) {
                    F::operator()(&a.weight);
                    it.first->second = a.weight;
                } else
                    a.weight = it.first->second;
            }
        }
    };

    Arcs arcs;
    int size;
#ifdef BIDIRECTIONAL
    int hitcount;			// how many times index is used, negative for index on input, positive for index on output
#endif
    typedef HashTable<IntKey, List<HalfArc> > Index;

    template <class IOMap>
    void index_io(IOMap &m) const
    {
        for ( Arcs::const_iterator a=arcs.begin(),end=arcs.end() ; a != end ; ++a )
            m[IOPair(a->in,a->out)].push_back(const_cast<FSTArc*>(&*a));
    }

    Index *index;

//    typedef HashTable<
    State() : arcs(), size(0),
#ifdef BIDIRECTIONAL
              hitcount(0),
#endif
              index(NULL) { }
    State(const State &s): arcs(s.arcs),size(s.size) {
#ifdef BIDIRECTIONAL
        hitcount = s.hitcount;
#endif
//    if (s.index == NULL)
        index = (Index *) NULL ;
//    else
//     index = NEW Index(*s.index);
    }
    ~State() { flush(); }

    void raisePower(double exponent=1.0)
    {
        for ( Arcs::val_iterator l=arcs.val_begin(),end=arcs.val_end() ; l != end ; ++l )
            l->weight.raisePower(exponent);
    }


    // input => left projection, output => right.  epsilon->string or string->epsilon (identity_fsa=true), or string->string (identity_fsa=false)
    void project(int dir=input,bool identity_fsa=false)
    {
        flush();
        for ( Arcs::val_iterator l=arcs.val_begin(),end=arcs.val_end() ; l != end ; ++l )
            if (dir==output)
                l->in=identity_fsa?l->out:0;
            else
                l->out=identity_fsa?l->in:0;
    }
//fixme: push back
    void indexBy(int dir = 0) {
        List<HalfArc> *list;
        if ( dir ) {
#ifdef BIDIRECTIONAL
            if ( hitcount++ > 0 )
                return;
            hitcount = 1;
            delete index;
#else
            if ( index )
                return;
#endif
            index = NEW Index(size);
            for ( Arcs::val_iterator l=arcs.val_begin(),end=arcs.val_end() ;
                  l != end  ;
                  ++l ) {
// if you distrust ht[key], I guess: //#define QUEERINDEX
#ifdef QUEERINDEX
                if ( !(list = find_second(*index,(IntKey)l->out)) )
                    add(*index,(IntKey)l->out,
                        List<HalfArc>(&(*l)));
                else
                    list->push_front(&(*l));
#else
                (*index)[l->out].push_front(&(*l));
#endif
            }
            return;
        }
#ifdef BIDIRECTIONAL
        if ( hitcount-- < 0 )
            return;
        hitcount = -1;
        delete index;
#else
        if ( index )
            return;
#endif
        index = NEW Index(size);
        for ( Arcs::val_iterator l=arcs.val_begin(),end=arcs.val_end() ; l != end ; ++l ) {
#ifdef QUEERINDEX
            if ( !(list = find_second(*index,(IntKey)l->in)) )
                add(*index,(IntKey)l->in,
                    List<HalfArc>(&(*l)));
            else
                list->push_front(&(*l));
#else
            (*index)[l->in].push_front(&(*l));
#endif

        }
    }
    void flush() {
        if (index)
            delete index;
        index = NULL;
#ifdef BIDIRECTIONAL
        hitcount = 0;
#endif
    }

    // don't pass by value, expensive
    struct arc_adder
    {
        typedef Arcs::val_iterator I;
//        typedef Arcs::back_insert_iterator I;
//        I b;
        typedef dynamic_array<State> StateVector;
        StateVector &states; // only safe because we *know* that only shallow copies are made in moving lists around
        dynamic_array<I> bi;
        arc_adder(StateVector &states) : states(states)
                                       ,bi(states.size())
        {}
        void operator()(unsigned i,FSTArc const& arc)
        {
//            states[i].addArc(arc); return;
            State &s=states[i];
            I &b=bi.at_grow(i);
#if 0
            if (b.null())
                b=s.arcs.back_inserter();
            *b++=arc;
#else
            b=s.arcs.insert_after(b,arc);
#endif
            ++s.size;
        }
    };


    FSTArc & addArc(const FSTArc &arc)
    {
        arcs.push(arc);
        ++size;
        Assert(!index);
#ifdef DEBUG
        if (index) {
            std::cerr << "Warning: adding arc to indexed state.\n";
            delete index;
            index = NULL;
        }
#endif
        return arcs.top();
    }

    static inline void combine_arc_weight(Weight &w,Weight d,bool sum,bool clamp)
    {
        if (sum) {
            w+=d;
            if (clamp && w>1)
                w.setOne();
        } else {
            if (d>w)
                w=d;
        }
    }

    void reduce(bool sum=true,bool clamp=false) {		// consolidate all duplicate arcs
        flush();
        HashTable<UnArc, Weight *> hWeights;
        UnArc un;
        Weight **ppWt;
        for ( Arcs::erase_iterator l=arcs.erase_begin(),end=arcs.erase_end() ; l != end ;) {
            if ( l->weight.isZero() ) {
                l=remove(l);
                continue;
            }
            un.in = l->in;
            un.out = l->out;
            un.dest = l->dest;
            if ( (ppWt = find_second(hWeights,un)) ) {
                combine_arc_weight(**ppWt,l->weight,sum,clamp);
                l=remove(l);
            } else {
                hWeights[un]= &l->weight; // add?
                ++l;
            }
        }
    }
    void remove_epsilons_to(int dest)  // *e*/*e* transition to same state has no structural/bestpath value.  we do this always when reducing (not "-d")
    {
        flush();
        for ( Arcs::erase_iterator a=arcs.erase_begin(),end = arcs.erase_end() ; a != end; )
            if ( a->in == 0 && a->out == 0 && a->dest == dest ) // erase empty loops
                a=remove(a);
            else
                ++a;
    }

    void prune(Weight thresh) {
        for ( Arcs::erase_iterator l=arcs.erase_begin(),end=arcs.erase_end() ; l != end ;) {
            if ( l->weight < thresh ) {
                l=remove(l);
            } else
                ++l;
        }
    }
    template <class T>
    T remove(T t) {
        --size;
        return arcs.erase(t);
    }
    void renumberDestinations(int *oldToNew) { // negative means remove transition
        flush();
        for (Arcs::erase_iterator l=arcs.erase_begin(),end=arcs.erase_end(); l != end; ) {
            int &dest = (int &)l->dest;
            if ( oldToNew[dest] < 0 ) {
                l=remove(l);
            } else {
                dest = oldToNew[dest];
                ++l;
            }
        }
    }


    void swap(State &b)
    {
        using std::swap;
        swap(size,b.size);
        swap(arcs,b.arcs);
        swap(index,b.index); // safe only because List iterators are stable when lists are swapped
#ifdef BIDIRECTIONAL
        swap(hitcount,b.hitcount);
#endif
    }
};


inline void swap(State &a,State &b)
{
    a.swap(b);
}


std::ostream& operator << (std::ostream &out, State &s); // Yaser 7-20-2000

}//ns


#endif
