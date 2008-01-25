#ifndef GRAEHL_CARMEL_DERIVATIONS_H
#define GRAEHL_CARMEL_DERIVATIONS_H

#ifdef DEBUGDERIVATIONS
# define DEBUG_DERIVATIONS_PRUNE
#endif 

/*

special form of compose: input string * transducer * output string (more efficient than compose.h (input string * transducer) * output string.  
*/
#include <boost/utility.hpp>
#include <graehl/shared/myassert.h>
#include <graehl/shared/hashtable_fwd.hpp>
#include <graehl/shared/graph.hpp>
#include <graehl/carmel/src/fst.h>
#include <graehl/carmel/src/train.h>
#include <graehl/shared/dynarray.h>
#include <array.hpp>
#include <io.hpp>

namespace graehl {

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


struct arcs_table : public dynamic_array<arc_counts>
{
    typedef dynamic_array<arc_counts> arcs_type;
    unsigned n_states;
    bool per_arc_prior;
    Weight global_prior;
    arcs_table(WFST & x,bool per_arc_prior=false,Weight global_prior=1.)
        : per_arc_prior(per_arc_prior),global_prior(global_prior)
    {
        n_states=x.numStates();
        x.visit_arcs(*this);
    }
    void operator()(unsigned s,FSTArc &a) 
    {
        this->push_back();
        arc_counts &ac=this->back();
        ac.arc=&a;
        ac.prior_counts=global_prior;
        if (per_arc_prior)
            ac.prior_counts+=a.weight;
        ac.src=s;
    }
    
#define ARCS_TABLE_EACH(i) for (arcs_type::iterator i=this->begin(),end=this->end();i!=end;++i)
#define ARCS_TABLE_EACH_C(i) for (arcs_type::const_iterator i=this->begin(),end=this->end();i!=end;++i)

    template <class V>
    void visit(V &v) 
    {
        ARCS_TABLE_EACH(i)
            v(*i);
    }

    template <class V>
    void visit(V const& v) 
    {
        ARCS_TABLE_EACH(i)
            v(*i);
    }

    template <class V>
    void visit(V &v) const
    {
        ARCS_TABLE_EACH_C(i)
            v(*i);
    }

    template <class V>
    void visit(V const& v)  const
    {
        ARCS_TABLE_EACH_C(i)
            v(*i);
    }    
    
    void dump(std::ostream &o,char const* header="") const 
    {
        o << "\n" << header << "\n";
        ARCS_TABLE_EACH_C(i)
            o << *i;
    }
    
#undef ARCS_TABLE_EACH_C
#undef ARCS_TABLE_EACH
};
    
struct wfst_io_index : boost::noncopyable
{
    typedef dynamic_array<unsigned> for_io; // id in arcs_table
    typedef HashTable<IOPair,for_io> for_state;
    typedef fixed_array<for_state> states_t;

    states_t st;
    arcs_table &t;
        
    wfst_io_index(arcs_table &t) : st(t.n_states),t(t)
    {
        for (unsigned i=0,N=t.size();i!=N;++i) {
            arc_counts const& ac=t[i];
            st[ac.src][IOPair(ac.in(),ac.out())].push_back(i);
        }
    }
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

    typedef GraphState::arcs_type arcs_type;

    struct reversed_graph 
    {
        fixed_array<GraphState> b;
        template <class Graph>
        reversed_graph(Graph const& forward) : b(forward.size())
        {
            add_reversed_arcs(&b.front(),&forward.front(),forward.size());
        }
        
        /// m[i] is set to val if state leads to i along a path in b, the reversed graph (i.e. in the orig. graph, something leads to "state".  you must set all m[i] !=val if they haven't already been visited by this
        template <class Mark,class Val>
        void mark_reaching(Mark &m,unsigned state,Val v) const
        {
            if (m[state]==v)
                return;
            m[state]=v;
            arcs_type const& arcs=b[state].arcs;
            for (arcs_type::const_iterator i=arcs.begin(),e=arcs.end();i!=e;++i)
                mark_reaching(m,i->dest,v);
        }
        Graph graph()
        {
            Graph ret;
            ret.states=&b.front();
            ret.nStates=b.size();
            return ret;
        }
        
    };
    
        
    
    
    void prune() 
    {
        if (empty())
            return;
        fixed_array<bool> remove(true,g.size());
#ifdef DEBUG_DERIVATIONS_PRUNE
        Config::debug() << "Pruning derivations - original:\n";
        printGraph(forward(),Config::debug());
#endif        
        reversed_graph r(g);
#ifdef DEBUG_DERIVATIONS_PRUNE
        Config::debug() << "Pruning derivations - reversed:\n";
        printGraph(r.graph(),Config::debug());
#endif        
        r.mark_reaching(remove,final(),false);
#ifdef DEBUG_DERIVATIONS_PRUNE
        Config::debug() << "Removing states:";
        print_range(Config::debug(),remove.begin(),remove.end());
        Config::debug() << "\n";
#endif  
        indices_after_removing ttable(remove);
        fin=ttable[fin];
        // now: remove[i] = true -> no path to final
        unsigned new_size=graehl::shuffle_removing(&g.front(),ttable,rewrite_GraphState());
#ifdef DEBUG_DERIVATIONS_PRUNE
        Config::debug()<<"old state->new state ttable:\n";
        print_range(Config::debug(),ttable.map,ttable.map+ttable.n_mapped);
        Config::debug()<<"\n";
                
        Config::debug() << "Pruning derivations - final, new size="<<new_size<<":\n";
                
        printGraph(forward(),Config::debug());
#endif        
        g.resize(new_size);
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
            for (for_io::const_iterator i=match->begin(),e=match->end();i!=e;++i) {
                unsigned id=*i;
                arc_counts const& a=io.t[id];
                g[source].add(source,derive(io,deriv_state(i_in,a.dest(),i_out)),0,(void *)id);
// note: had to use g[source] rather than caching the iterator, because recursion may invalidate any previously taken iterator
            }
        
    }
    
    
};


}

BEGIN_HASH_VAL(graehl::deriv_state) {	return x.hash(); } END_HASH

namespace graehl {

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



#endif
