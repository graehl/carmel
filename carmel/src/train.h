#ifndef CARMEL_TRAIN_H
#define CARMEL_TRAIN_H

#include <graehl/shared/word_spacer.hpp>
#include <graehl/shared/config.h>
#include <graehl/shared/myassert.h>
#include <graehl/shared/2hash.h>
#include <graehl/shared/weight.h>
#include <graehl/shared/list.h>
#include <graehl/shared/dynarray.h>
#include <graehl/shared/array.hpp>
#include <graehl/shared/arc.h>
#include <iostream>

namespace graehl {

void check_fb_agree(Weight f,Weight b);
void training_progress(unsigned train_example_no);

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
    return l.in == r.in && l.out == r.out; }

}

BEGIN_HASH_VAL(graehl::IntKey) {	return x.hash(); } END_HASH
BEGIN_HASH_VAL(graehl::IOPair) {	return x.hash(); } END_HASH

namespace graehl {

struct State {
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
            fst.AddArc(source,A(a->in,a->out,W(a->weight.getNegLog10()),a->dest));
        }
    }
    
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
    
    
    void indexBy(int output = 0) {
        List<HalfArc> *list;
        if ( output ) {
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
    
    void reduce() {		// consolidate all duplicate arcs
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
                **ppWt += l->weight;
                if ( **ppWt > 1 )
                    **ppWt = Weight((FLOAT_TYPE)1.);
	
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

struct arc_counts 
{
    unsigned src;
    unsigned dest() const
    {
        return arc->dest;
    }
    int in() const
    {
        return arc->in;
    }
    int out() const
    {
        return arc->out;
    }
    int groupId() const 
    {
        return arc->groupId;
    }
    
    FSTArc *arc;
    Weight scratch;
    Weight em_weight;
    Weight best_weight;
    Weight counts;
    Weight prior_counts;
    Weight &weight() const 
    {
        return arc->weight;
    }
};

std::ostream& operator << (std::ostream &o,arc_counts const& ac);

    

struct DWPair {    
    unsigned dest; // to allow reverse forward = backward version
    unsigned id; // to arc_counts table
    DWPair(DWPair const& o) : dest(o.dest),id(o.id) {}
    DWPair() {}
    DWPair(unsigned dest,unsigned id) : dest(dest),id(id) {}
};

std::ostream & operator << (std::ostream &o, DWPair p);


std::ostream & hashPrint(HashTable<IOPair, List<DWPair> > &h, std::ostream &o);

struct symSeq {
    int n;
    int *let;
    int *rLet;
    typedef int* iterator;
    typedef int*const_iterator;
    iterator begin() const
    {
        return let;
    }
    iterator end() const
    {
        return let+n;
    }
    iterator rbegin() const
    {
        return rLet;
    }
    iterator rend() const
    {
        return rLet+n;
    }
    unsigned size() const
    {
        return n;
    }
    template <class O,class Alphabet>
    void print(O &o,Alphabet const& a) const
    {
        graehl::word_spacer sp;
        for (int i=0;i<n;++i)
            o << sp << a[let[i]];
    }
};

std::ostream & operator << (std::ostream & out , const symSeq & s);

struct IOSymSeq {
    symSeq i;
    symSeq o;
    FLOAT_TYPE weight;
    IOSymSeq(List<int> const&inSeq, List<int> const&outSeq, FLOAT_TYPE w) 
    {
        init(inSeq,outSeq,w);
    }
    template <class S>
    void init(S const&inSeq, S const&outSeq, FLOAT_TYPE w) {
        i.n = inSeq.size();
        o.n = outSeq.size();
        i.let = NEW int[i.n];
        o.let = NEW int[o.n];
        i.rLet = NEW int[i.n];
        o.rLet = NEW int[o.n];
        int *pi, *rpi;
        pi = i.let;
        rpi = i.rLet + i.n;
        for ( typename S::const_iterator inL=inSeq.begin(),endI=inSeq.end() ; inL != endI ; ++inL )
            *pi++ = *--rpi = *inL;
        pi = o.let;
        rpi = o.rLet + o.n;
        for ( typename S::const_iterator outL=outSeq.begin(),endO=outSeq.end() ; outL != endO ; ++outL )
            *pi++ = *--rpi = *outL;
        weight = w;
    }
    void kill() {
        if (o.let) {
            delete[] o.let;
            delete[] o.rLet;
            delete[] i.let;
            delete[] i.rLet;
            o.let=NULL;
        }
    }
    IOSymSeq(IOSymSeq const& o) 
    {
        init(o.i,o.o,o.weight);
    }
    
    ~IOSymSeq() 
    {
        kill();
    }
    
    template <class O,class Alphabet>
    void print(O &os,Alphabet const& in,Alphabet const& out,char const* term="\n") const
    {
        i.print(os,in);
        os << term;
        o.print(os,out);
        os << term;
    }
    template <class O,class FST>
    void print(O &o,FST const& x,char const* term="\n") const
    {
        print(o,x.in_alph(),x.out_alph(),term);
    }
    
};

std::ostream & operator << (std::ostream & out , const IOSymSeq & s);   // Yaser 7-21-2000

class training_corpus : boost::noncopyable
{
 public:
    training_corpus() { clear(); }
    void clear()
    {
        maxIn=maxOut=0;
        n_input=n_output=0;
        w_input=w_output=totalEmpiricalWeight=0;
        examples.clear();
    }
    
    void add(List<int> &inSeq, List<int> &outSeq, FLOAT_TYPE weight=1.) 
    {
        examples.push_front(inSeq,outSeq,weight);
        IOSymSeq const& n=examples.front();
        unsigned i=n.i.n,o=n.o.n;
        n_input += i;
        n_output += o;
        w_input += weight*i;
        w_output += weight*o;
        if (maxIn<i) maxIn=i;
        if (maxOut<o) maxOut=o;
        totalEmpiricalWeight += weight;
    }
    void finish_adding() 
    {
        examples.reverse();
    }
    void set_null()
    {
        clear();
        List<int> empty_list;
        add(empty_list, empty_list, 1.0);
    }    

//    bool cache_derivations;
    unsigned maxIn, maxOut; // highest index (N-1) of input,output symbols respectively.
    List <IOSymSeq> examples;
    //Weight smoothFloor;
    FLOAT_TYPE totalEmpiricalWeight; // # of examples, if each is weighted equally
    FLOAT_TYPE n_input,n_output,w_input,w_output; // for per-symbol ppx.  w_ is multiplied by example weight.  n_ is unweighted
};

}



#endif
