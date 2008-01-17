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
    unsigned i,s,o; // input,state,output
    std::size_t hash() const 
    {
        return hash_quads_64(&i,sizeof(deriv_state)/sizeof(i));
//hash_bytes_32((void *)this,sizeof(deriv_state));
    }
    bool operator ==(deriv_state const& r)  const
    {
        return r.i==i && r.s==s && r.o==o;
    }
    deriv_state(unsigned i,unsigned s,unsigned o) : i(i),s(s),o(o) {}
};

struct wfst_io_index 
{
    typedef dynamic_array<HalfArc> for_io; // HalfArc = FstArc *
    typedef HashTable<IOPair,for_io> for_state;
    typedef fixed_array<for_state> states_t;

    states_t st;
    
        
    wfst_io_index(WFST & x) : st(x.numStates())
    {
        for (unsigned s=0;s<x.numStates();++s)
            x.states[s].index_io(st[s]);
    }
 private:
    wfst_io_index(wfst_io_index const& o) { *(int*)0; }
};

    
struct derivations
{
 private:
//    typedef List<int> Symbols;
    typedef int Sym;
    typedef dynamic_array<Sym> Seq;
    WFST const& x;
    
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
    derivations(WFST const & x,Symbols const& in,Symbols const& out) // unusually, you must call compute() before using the object.  lets me push_back easier in container w/o copy or pimpl
        : x(x)
        ,in(in.begin(),in.end())
        ,out(out.begin(),out.end())
    {}

//    derivations(derivations const& o) : x(o.x),in(o.in),out(o.out) {} // similarly, this doesn't really copy the derivations; you need to compute() after.  um, this would be bad if you used a vector rather than a list?

    // return true iff goal reached (some deriv exists)
    bool compute(wfst_io_index const& io,bool drop_names=true,bool prune_=true) 
    {
        state_id start=derive(io,deriv_state(0,0,0));
        assert(start==0);
        deriv_state goal(in.size(),x.final,out.size());
        state_id *pfin=find_second(id_of_state,goal);
        if (pfin)
            fin=*pfin;
        no_goal=(pfin==NULL);
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
    state_id derive (wfst_io_index const& io,deriv_state const& d);
    
    void add_arcs(wfst_io_index const& io,Sym s_in,Sym s_out,unsigned i_in,unsigned i_out
                  ,wfst_io_index::for_state const& fs,unsigned source) 
    {
        typedef wfst_io_index::for_io for_io;
        if (for_io const* match=find_second(fs,IOPair(s_in,s_out)))
            for (for_io::const_iterator i=match->begin(),e=match->end();i!=e;++i)
                g[source].add(source,derive(io,deriv_state(i_in,(*i)->dest,i_out)),0,*i);
// note: g[source] rather than caching the iterator, because recursion may invalidate any previously taken iterator        
    }
    
    
};

}
}

BEGIN_HASH_VAL(graehl::carmel::deriv_state) {	return x.hash(); } END_HASH

namespace graehl {
namespace carmel {

derivations::state_id derivations::derive(wfst_io_index const& io,deriv_state const& d) 
{
    const int EPS=WFST::epsilon_index;
    state_id ret=g.size();
    
    state_to_id::insert_return_type already=
        id_of_state.insert(d,ret);
    if (!already.second)
        return already.first->second;

    
    add(id_of_state,d,ret); // NOTE: very important that we've added this before we start taking self-epsilons.
    g.push_back();
        
    wfst_io_index::for_state const&fs=io.st[d.s];

        
    add_arcs(io,EPS,EPS,d.i,d.o,fs,ret);
    
    
    if (d.o < out.size()) {
        add_arcs(io,EPS,out[d.o],d.i,d.o+1,fs,ret);
    }
    
    if (d.i < in.size()) {
        Sym i=in[d.i];
        add_arcs(io,i,EPS,d.i+1,d.o,fs,ret);
        if (d.o < out.size()) {
            add_arcs(io,i,out[d.o],d.i+1,d.o+1,fs,ret);
        }
    }

    return ret;
    
}


}
}


#endif
