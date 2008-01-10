#ifndef GRAEHL_CARMEL_DERIVATIONS_H
#define GRAEHL_CARMEL_DERIVATIONS_H

/*

special form of compose: input string * transducer * output string (more efficient than compose.h (input string * transducer) * output string.  
*/
#include <graehl/shared/myassert.h>
#include <graehl/shared/hashtable_fwd.hpp>
#include <graehl/shared/graph.hpp>
#include <graehl/carmel/src/fst.h>
#include <graehl/carmel/src/train.h>
#include <graehl/shared/dynarray.h>


namespace graehl {
namespace carmel {

struct deriv_state 
{
    int i,s,o; // input,state,output
    uint32_t hash() const 
    {
        return hash_bytes_32((void *)this,sizeof(deriv_state));
    }
    bool operator ==(deriv_state const& r)  const
    {
        return r.i==i && r.s==s && r.o==o;
    }
    deriv_state(int i,int s,int o) : i(i),s(s),o(o) {}
};

struct wfst_io_index 
{
    WFST const& x;    
    
    typedef dynamic_array<HalfArc> for_io; // HalfArc = FstArc *
    typedef HashTable<IOPair,for_io> for_state;
    typedef auto_array<for_state> states_t;

    states_t st;
    
        
    wfst_io_index(WFST & xfst) : x(xfst), st(xfst.numStates())
    {
        for (unsigned s=0;s<x.numStates();++s)
            xfst.states[s].index_io(st[s]);
    }

};

    
struct derivations
{
 private:
//    typedef List<int> Symbols;
    typedef int Sym;
    typedef dynamic_array<Sym> Seq;
    wfst_io_index const& x;
    Seq in,out;
    
    typedef dynamic_array<GraphState> vgraph;
    vgraph g;
    typedef unsigned state_id;
    state_id fin;
    bool no_goal;
    
    typedef HashTable<deriv_state,state_id> state_to_id;
    state_to_id id_of_state;
 public:
    bool empty() const 
    {
        return no_goal;
    }
    state_id final() const 
    {
        assert(!empty());
        return fin;
    }
    
    Graph forward() 
    {
        assert(!empty());
        
        Graph r;
        r.states=&g.front();
        r.nStates=g.size();
        return r;
    }
    
    template <class Symbols>
    derivations(WFST & xfst,Symbols const& in,Symbols const& out) // unusually, you must call compute() before using the object.  lets me push_back easier in container w/o copy or pimpl
        : x(xfst)
        ,in(in.begin(),in.end())
        ,out(out.begin(),out.end())
    {}

    derivations(derivations const& o) : x(o.x),in(o.in),out(o.out) {} // similarly, this doesn't really copy the derivations; you need to compute() after.  um, this would be bad if you used a vector rather than a list?

    // return true iff goal reached (some deriv exists)
    bool compute(bool drop_names=true,bool prune_=true) 
    {
        state_id start=derive(deriv_state(0,0,0));
        assert(start==0);
        deriv_state goal(in.size(),x.x.final,out.size());
        state_id *pfin=find_second(id_of_state,goal);
        if (pfin)
            fin=*pfin;
        no_goal=!pfin;
        if (drop_names)
            id_of_state.clear();
        if (prune_)
            prune();
        return !no_goal;
    }

    void prune() 
    {
        if (empty())
            return;
        //TODO, not necessary
    }
    
 private:
    /*
    Graph reverse() //WARNING: you have to delete[] return.states 
    {
        return reverseGraph(forward());
    }
    */
    state_id derive (deriv_state const& d);
    
};

}
}

BEGIN_HASH_VAL(graehl::carmel::deriv_state) {	return x.hash(); } END_HASH

namespace graehl {
namespace carmel {

derivations::state_id derivations::derive(deriv_state const& d) 
{
    const int EPS=WFST::epsilon_index;
    
    {    
        state_id *already;
        if ((already=find_second(id_of_state,d)))
            return *already;
    }
    
    state_id ret=g.size();
    add(id_of_state,d,ret); // NOTE: very important that we've added this before we start taking self-epsilons.
    g.push_back();
    GraphState &from=g.back();
    GraphArc arc;
    arc.source=ret;
    arc.weight=0;
        
    wfst_io_index::for_state const&fs=x.st[d.s];


    typedef wfst_io_index::for_io for_io;
    for_io const* match;
    
#define TRYARC(s_in,s_out,i_in,i_out) \
    if (match=find_second(fs,IOPair(s_in,s_out))) { \
        for_io const& m=*match;                        \
        for (for_io::const_iterator i=m.begin(),e=m.end();i!=e;++i) { \
            HalfArc h=*i; \
            arc.data=h; \
            arc.dest=derive(deriv_state(i_in,h->dest,i_out)); \
            from.add(arc); \
        } \
    } /* END TRYARC */
    
    TRYARC(EPS,EPS,d.i,d.o); 

    if (d.o < out.size()) {
        TRYARC(EPS,out[d.o],d.i,d.o+1);
    }
    
    if (d.i < in.size()) {
        Sym i=in[d.i];
        TRYARC(i,EPS,d.i+1,d.o);
        if (d.o < out.size()) {
            TRYARC(i,out[d.o],d.i+1,d.o+1);
        }
    }

    return ret;
    
}


}
}


#endif
