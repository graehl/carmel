#ifndef GRAEHL_CARMEL__DERIVATIONS_H
#define GRAEHL_CARMEL__DERIVATIONS_H

#define DERIVPRUNE 1
//FIXME: figure out if warning about pruning not allowing goal detection still applies
//#define DERIVPRUNE 0

#ifdef DEBUGDERIVATIONS
# define DEBUG_DERIVATIONS_PRUNE
//# define DEBUG_DERIVATIONS_PRUNE_EXTRA
#endif

/*

special form of compose: input string * transducer * output string (more efficient than compose.h (input string * transducer) * output string.

NOTE: grapharc data field is an unsigned index into an arcs_table (which has the fstarc * and possibly ancillary counts).  for gibbs+cascade, link directly to cascade would be sufficient.

believe/check: only one (composed) arc between any 2 states, because each state is cascade-state-tuple,i-pos,o-pos.  change in i,o pos determines label of arc.  with -a composition, that means only one cascade arc between any 2 deriv states.

*/
//#define GRAEHL__DEBUG_print
#include <graehl/shared/debugprint.hpp>
#include <graehl/shared/weight.h>
#include <boost/utility.hpp>
#include <graehl/shared/myassert.h>
#include <graehl/shared/hashtable_fwd.hpp>
#include <graehl/shared/graph.h>
#include <graehl/shared/simple_serialize.hpp>
#include <graehl/carmel/src/fst.h>
#include <graehl/carmel/src/train.h>
#include <graehl/shared/dynarray.h>
#include <boost/cstdint.hpp>
//#include <graehl/shared/stream_util.hpp>
//#include <graehl/shared/size_mega.hpp>

#include <array.hpp>
#include <io.hpp>

namespace graehl {

//TODO: epsilon filter needed so training sums over subseqs of *e*:o and i:*e* don't care about order?  don't think so.
struct deriv_state
{
    typedef deriv_state self_type;
    TO_OSTREAM_PRINT
    template<class O>
    void print(O &os) const
    {
        os<<'(';
        os<<i<<' '<<s<<' '<<o;
        os<<')';
    }
    uint32_t i,s,o; // input,state,output
    uint32_t hash() const
    {
        //return hash_quads_64(&i,sizeof(deriv_state)/sizeof(i));
        return hash3(i,s,o);
//hash_bytes_32((void *)this,sizeof(deriv_state));
    }
  MEMBER_HASH
    bool operator !=(deriv_state const& r)  const
    {
        return !(*this==r);
    }
    bool operator ==(deriv_state const& r)  const
    {
        return r.i==i && r.s==s && r.o==o;
    }

    deriv_state() {  }
    deriv_state(uint32_t i,uint32_t s,uint32_t o) : i(i),s(s),o(o) {}
};

//inline uint32_t hash_value(deriv_state const& d) { return d.hash(); }
}
// above would suffice for boost::hash, but we're still 'flexible' and want to put in the same namespace as hashtable impl so it gets found by default, so:

//BEGIN_HASH_VAL(graehl::deriv_state) {	return x.hash(); } END_HASH


namespace graehl {

// temporary counts etc. for deriv arcs.  see cascade.h for how weights are pushed back to original wfsts
template <class arc_counts>
struct arcs_table : public dynamic_array<arc_counts>
{
    typedef arc_counts counts_type;
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
    arc_counts &ac(GraphArc const& a) const
    {
        assert(a.data_as<unsigned>()<this->size());
        return (*(arcs_type *)this)[a.data_as<unsigned>()];
    }

    void operator()(unsigned s,FSTArc &a)
    {
        this->push_back();
        arc_counts &ac=this->back();
        ac.set(s,&a,per_arc_prior?global_prior+a.weight:global_prior); //TODO: different prior per transducer
    }

#define ARCS_TABLE_EACH(i) for (typename arcs_type::iterator i=this->begin(),end=this->end();i!=end;++i)
#define ARCS_TABLE_EACH_C(i) for (typename arcs_type::const_iterator i=this->begin(),end=this->end();i!=end;++i)

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
    typedef arc_counts counts_type;
    typedef dynamic_array<unsigned> for_io; // id in arcs_table
    typedef HashTable<IOPair,for_io> for_state;
    typedef fixed_array<for_state> states_t;

    states_t st;

/*    wfst_io_index(arct &t) : st(t.n_states),t(t)
    {
        for (unsigned i=0,N=t.size();i!=N;++i) {
            arc_counts const& ac=t[i];
            st[ac.src][IOPair(ac.in(),ac.out())].push_back(i);
        }
    }
*/
    wfst_io_index(WFST const&x) : st(x.numStates())
    {
        i=0;
        ((WFST&)x).visit_arcs(*this);
    }
    unsigned i; // # arcs
    void operator()(unsigned src,FSTArc &a)
    {
        st[src][IOPair(a.in,a.out)].push_back(i++);
    }
};


// all the derivations for one in/out pair through WFST x
// note that the reverse graph and topo (cache_backward saves them) point to arcs directly via pointers, so serialization omits those (forces cache_backward=false on load), so they're rebuilt when needed
// containing derivations in a dynarray is fine since the lists used in graph states are movable, and state_to_id isn't kept around after compute, but even if it is, HashTable can also be moved.
struct derivations //: boost::noncopyable
{

 private:
//    typedef List<int> Symbols;
    typedef int Sym;
    typedef dynamic_array<Sym> Seq;

    Seq in,out; // cleared after compute()

    typedef dynamic_array<GraphState> vgraph;
    vgraph g;
    typedef unsigned state_id;
    state_id fin;
    bool no_goal;
    bool cache_backward;

    typedef HashTable<deriv_state,state_id> state_to_id;
    state_to_id id_of_state;
    typedef fixed_array<deriv_state> id_to_state;
    void fill_id_to_state(id_to_state &f)
    {
        f.reinit(g.size());
        for (state_to_id::iterator i=id_of_state.begin();i!=id_of_state.end();++i)
            f[i->second]=i->first;
    }
 public:

    Weight n_paths() const
    {
        if (empty()) return zero_weight();
        return countNoCyclePathsTo<Weight>(graph(),0,fin);
    }

    struct statistics
    {
        Weight sum_paths;
        Weight prod_paths;
        double N;
        statistics() : prod_paths(one_weight()), N(0),pruned(false) {  }
        bool pruned;

        void prune_record(derivations &d,bool do_prune=true)
        {
            pruned=do_prune;
            ++N;
            pre.states=d.g.size();
            if (do_prune)
                d.prune();
            else
                post=pre;
            Weight npath=d.n_paths();
            prod_paths*=npath;
            sum_paths+=npath;
        }

        struct states_arcs
        {
            double states,arcs;

            states_arcs() {
                clear();
            }
            void clear()
            {
                states=arcs=0;
            }
            void print(std::ostream &o) const
            {
                o << "(" << states << " states, "<<arcs<<" arcs)";
            }
            void operator /= (states_arcs const& d)
            {
                states/=d.states;
                if (d.arcs==0)
                    arcs=1;
                else
                    arcs/=d.arcs;
            }
            typedef states_arcs self_type;
            TO_OSTREAM_PRINT
        };

        states_arcs pre,post;

        void print(std::ostream &o) const
        {
            states_arcs ratio=post;
            ratio/=pre;

            o << "\nTotal for "<<N<<" cached derivations:";
            if (!pruned)
                o << " " << pre << "\n";
            else
                o << "\n"
                  <<  "Pre pruning: "<< pre <<"\n"
                  <<"Post pruning: " << post << "\n"
                  << "Portion kept: " << ratio << "\n"
                  << "Avg # of paths (arith. mean): " << sum_paths/N << "\n"
                  << "Avg # of paths (geom. mean): " << prod_paths.root(N) << "\n"
                    ;
        }
        typedef statistics self_type;
        TO_OSTREAM_PRINT
    };

    static statistics global_stats;

    double weight;
    unsigned lineno;

    bool empty() const
    {
        return no_goal;
    }
    state_id start() const
    {
        return 0;
    }
    state_id final() const
    {
        assert(!empty());
        return fin;
    }

    unsigned n_deriv_fsa_states() const
    {
        return g.size();
    }

    /*
    void update_weights(arcs_table &t)
    {
        for (vgraph::iterator i=g.begin(),e=g.end();i!=e;++i) {
            unsigned arcid=i->data_as<unsigned>();
            i->weight=t[arcid].weight().getReal();
        }
    }
    */

    unsigned n_states() const
    {
        return g.size();
    }

    Graph graph() const
    {
        assert(!empty());

        Graph r;
        r.states=const_cast<GraphState *>(&g.front());
        r.nStates=g.size();
        return r;
    }

    derivations() {}

    // for EM, not gibbs:
    template <class arcs_table>
    struct weight_for
    {
        typedef typename arcs_table::counts_type arc_counts;
        typedef Weight result_type;
        arcs_table const& t;
        weight_for(arcs_table const& t) : t(t) {}
        arc_counts &ac(GraphArc const& a) const
        {
/*            assert(a.data_as<unsigned>()<t.size());
              return t[a.data_as<unsigned>()];*/
            return t.ac(a);
        }
        Weight operator()(GraphArc const& a) const
        {
            return t.ac(a).weight();
        }
    };

    //FIXME: allow storying r.graph() as primary, free up graph() (for gibbs)

    typedef fixed_array<Weight> fb_weights;

#define ORANDPATH(x) //std::cerr<<x
    // weights for random_path (for gibbs).
    template <class WeightFor>
    struct pfor
    {
        fb_weights b;
        WeightFor const &wf;
        pfor(unsigned nst,unsigned fin,WeightFor const &wf) : b(nst),wf(wf)//
        {
            b[fin]=1;
        }
        // store in GraphArc .weight the normalized probability
        template <class It>
        void global_normalize(It beg,It end,double power=1.) const
        {
            Weight sum;
            ORANDPATH("global_normalize power="<<power);
            for (It i=beg;i!=end;++i) {
                GraphArc & a=*i;
                Weight nw=(wf(a)*b[a.dest]).pow(power); // req 1: wf(a)
                ORANDPATH(" p="<<a.wt()<<" b="<<b[a.dest]);
                sum+=nw;
                a.wt()=nw;
            }
            ORANDPATH(" =>(globalnorm sum="<<sum<<")");
            if (sum.isZero())
                sum.setOne();
            for (It i=beg;i!=end;++i) {
                GraphArc & a=*i;
                a.weight=(a.wt()/sum).getReal(); // warning: aliasing (a.wt() and a.weight are a fake-union)
                ORANDPATH(" "<<a.weight);
            }
            ORANDPATH("\n");
        }
        //used by random.hpp choose_p
        double operator()(GraphArc const& a) const
        {
            return a.weight;
        }
    };

    // Weight wf(GraphArc &a)
    // wf.choose_arc(GraphArc const& a)
    //  WeightFor is carmel_gibbs typically (or else p_init for EM init samples).  2 req listed below:
    template <class WeightFor>
    void random_path(WeightFor const& wf,double power=1.)
    {
        if (empty()) return;
        unsigned nst=g.size();
        get_order();
        get_reverse();
        Graph rg=r.graph();
        rg.setwt(wf); // req 1: wf(GraphArc a).  now a.wt() has the result
        pfor<WeightFor> pf(nst,fin,wf);
        propagate_paths_in_order_wt(rg,reverse_order.begin(),reverse_order.end(),pf.b); // uses wf(a) from setwt
        free_order();
        free_reverse();
        unsigned s=0;
        std::vector<bool> normed(nst,false); // this is linear time; may actually slow down random choice vs. normalizing on fly
        while (s!=fin) { // fin should have no outgoing arcs if you want sampling to be sensible
            arcs_type &arcs=g[s].arcs;
            if (!normed[s]) { // don't normalize (+compute probs for arcs) in states we don't visit, and save the result if we revisit
                normed[s]=true;
                pf.global_normalize(arcs.val_begin(),arcs.val_end(),power);
            }
            GraphArc const& a=*choose_p(arcs.const_begin(),arcs.const_end(),pf); // no empty states allowed that aren't final.
            // use the re-normalizing choose_p rather than choose_p01 to protect from rounding issues with Weight::getReal sum to 1
            wf.choose_arc(a); // req 2: wf.choose_arc(GraphArc a)
            s=a.dest;
        }
    }

  bool have_order;


    //FIXME: gibbs.cc / incremental em version of below - too lazy to refactor, fix later
    template <class gibbs>
    Weight collect_counts_gibbs(gibbs const& gi)
    {
        if (empty()) return 1;
//        update_weights(t);
        unsigned nst=g.size();
        fb_weights f(nst),b(nst); // default 0-init
        Weight prob=compute_fb(f,b,gi);
        for (unsigned s=0;s<nst;++s) {
            arcs_type const& arcs=g[s].arcs;
            for (arcs_type::const_iterator i=arcs.begin(),e=arcs.end();i!=e;++i) {
                GraphArc const& a=*i;
                Weight arc_contrib=gi(a)*f[a.src]*b[a.dest];
                gi.choose_arc(a,arc_contrib/prob); //TODO: VERY minor speedup: initialize b[fin] to 1/prob instead of 1?  then no need to /prob.
            }
        }
        return prob;
    }

  template <class V>
  Weight compute_fb(fb_weights &f,fb_weights &b,V &gi)
  {
    assert(!empty());
    unsigned nst=g.size();
    f.reinit(nst);
    b.reinit(nst);
    f[0]=1;
    get_order();
    propagate_paths_in_order(graph(),reverse_order.rbegin(),reverse_order.rend(),gi,f);
    Weight prob=f[fin];
    get_reverse();
    b[fin]=1;
    propagate_paths_in_order(r.graph(),reverse_order.begin(),reverse_order.end(),gi,b);
    free_order();
    free_reverse();
    check_fb_agree(prob,b[0]);
    return prob;
  }

  template <class arcs_table>
  Weight prob(arcs_table &t)
  {
    weight_for<arcs_table> wf(t);
    fb_weights f(g.size());
    f[0]=1;
    get_order();
    propagate_paths_in_order(graph(),reverse_order.rbegin(),reverse_order.rend(),wf,f);
    Weight prob=f[fin];
    free_order();
    return prob;
  }

    // update expected counts and return prob (sum of paths)
    template <class arcs_table>
    Weight collect_counts(arcs_table &t)
    {
//        update_weights(t);
        weight_for<arcs_table> wf(t);
        unsigned nst=g.size();
        fb_weights f(nst),b(nst); // default 0-init
        Weight prob=compute_fb(f,b,wf);
        for (unsigned s=0;s<nst;++s) {
            arcs_type const& arcs=g[s].arcs;
            for (arcs_type::const_iterator i=arcs.begin(),e=arcs.end();i!=e;++i) {
                GraphArc const& a=*i;
                arc_counts &ac=wf.ac(a);
                Weight arc_contrib=ac.weight()*f[a.src]*b[a.dest];
                ac.counts += arc_contrib*weight/prob;
            }
        }
        return prob;
    }


 private:
    derivations(derivations const& o) : in(o.in),out(o.out) {} // similarly, this doesn't really copy the derivations; you need to compute() after.  um, this would be bad if you used a vector rather than a list?
 public:
    // return true iff goal reached (some deriv exists)
    template <class Symbols>
    void init(Symbols const& in_,Symbols const& out_,double w=1,unsigned line=0,bool cache_backward_=false)
    {
        in.set(in_);
        out.set(out_);
        weight=w;
        lineno=line;
        cache_backward=cache_backward_;
        id_of_state.clear();
        g.clear();
        free_reverse();
    }

    template <class Symbols,class arcs_table>
    bool init_and_compute(WFST &x,wfst_io_index const& io,arcs_table const& atab,Symbols const& in_,Symbols const& out_,double w=1,unsigned line=0,bool cache_backward_=false,bool prune_=true,bool drop_names=true)
    {
        init(in_,out_,w,line,cache_backward_);
        return compute(x,io,atab,drop_names,prune_);
    }

    template <class arcs_table>
    bool compute(WFST &x,wfst_io_index const& io,arcs_table const&atab,bool drop_names=true,bool prune_=true)
    {
#if DERIVPRUNE
        remove.clear();
#endif
        goal=deriv_state(in.size(),x.final,out.size());
#ifdef DEBUG_DERIVATIONS_PRUNE_EXTRA
        Config::debug()<<"\ngoal="<<goal<<"\n";
#endif

        state_id start=derive(io,atab,deriv_state(0,0,0));
        assert(start==0);
        state_id *pfin=find_second(id_of_state,goal);
        if (pfin)
            fin=*pfin;
        no_goal=(pfin==NULL);
        if (no_goal) {
#if defined(DEBUGDERIVATIONS)
          Config::debug()<<"goal "<<goal<<" not found:\n";
          Config::debug()<<id_of_state<<'\n';
#endif
        }
        global_stats.prune_record(*this,prune_);
        if (drop_names)
            id_of_state.clear();
        if (no_goal) {
            g.clear();
            return false;
        } else {
            if (cache_backward) {
                make_reverse();
                make_order();
            }
            return true;
        }
    }

    // nonportable serialization to temporary rewindable tape file
    template <class A>
    void serialize(A &a)
    {
        a & fin & g & weight & lineno;
        if (A::is_loading) {
            no_goal=g.empty();
            free_extras(); // not saved/loaded, so clear
        }
    }

    void free_extras()  // no longer needed after compute
    {
        cache_backward=false;
        free_reverse();
        id_of_state.clear();
        in.clear();
        out.clear();
    }

    typedef GraphState::arcs_type arcs_type;

    struct reversed_graph
    {
        fixed_array<GraphState> b;
        reversed_graph() {}

        template <class Graph>
        reversed_graph(Graph const& forward)
        {
            init(forward);
        }

        // only call if you clear() or default constructed last
        template <class Graph>
        void init(Graph const& forward)
        {
            b.init(forward.size());
            add_reversed_arcs(&b.front(),&forward.front(),forward.size(),false);
        }

        void clear()
        {
            b.clear();
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
        Graph graph() const
        {
            Graph ret;
            ret.states=const_cast<GraphState *>(&b.front());
            ret.nStates=b.size();
            return ret;
        }
    };

    void prune()
    {
        if (empty())
            return;
#ifdef DEBUG_DERIVATIONS_PRUNE
        std::ostream &dbg=Config::debug();
#endif
#if !DERIVPRUNE
        fixed_array<bool> remove(true,g.size());
#ifdef DEBUG_DERIVATIONS_PRUNE_EXTRA
        Config::debug() << "Pruning derivations - original:\n";
        printGraph(graph(),Config::debug());
#endif
        reversed_graph r(g);
#ifdef DEBUG_DERIVATIONS_PRUNE_EXTRA
        Config::debug() << "Pruning derivations - reversed:\n";
        printGraph(r.graph(),Config::debug());
#endif
        r.mark_reaching(remove,final(),false);
#endif

#ifdef DEBUG_DERIVATIONS_PRUNE
        id_to_state i2s;
        fill_id_to_state(i2s);
//        dbg<<"\ngoal="<<goal<<"\n";
        for (unsigned i=0,N=remove.size();i!=N;++i) {
            bool r=remove[i],r2=this->remove[i];
# if DEBUG_DERIVATIONS_PRUNE_EXTRA
            if (!r)
                dbg<<"i="<<i<<" "<<(r?'*':'.')<<" "<<(r2?'*':'.')<<" "<<i2s[i]<<'\n';
# endif
            if (r!=r2)
                Config::warn()<<"state="<<i<<" dfs="<<(r?'*':'.')<<" build="<<(r2?'*':'.')<<i2s[i]<<'\n';
        }
        dbg<<"\ngoal="<<goal<<"\n";
#endif

#ifdef DEBUG_DERIVATIONS_PRUNE_EXTRA
        Config::debug() << "Removing states:";
        print_range(Config::debug(),remove.begin(),remove.end());
        Config::debug() << "\n";
#endif
        indices_after_removing ttable(remove);
        remove.clear();
        fin=ttable[fin];
        rewrite_GraphState rw;
        unsigned new_size=graehl::shuffle_removing(&g.front(),ttable,rw);
        global_stats.post.states=new_size;
        global_stats.post.arcs=rw.n_kept;
#ifdef DEBUG_DERIVATIONS_PRUNE_EXTRA
        Config::debug()<<"old state->new state ttable:\n";
        print_range(Config::debug(),ttable.map,ttable.map+ttable.n_mapped);
        Config::debug()<<"\n";

        Config::debug() << "Pruning derivations - final, new size="<<new_size<<":\n";
        printGraph(graph(),Config::debug());
#endif
        g.resize(new_size);
        g.compact();
    }

 private:
//    typedef std::vector<bool> remove_t; // bit vector
    typedef dynamic_array<bool> remove_t;
#if DERIVPRUNE
    remove_t remove; // remove[i] = true -> no path to final
#endif
    deriv_state goal;

    //TODO: integrate prune+derive?  win=won't have to add an arc that doesn't finish.
    template <class arcs_table>
    state_id derive(wfst_io_index const& io,arcs_table const&atab,deriv_state const& d)
    {
        const int EPS=WFST::epsilon_index;
        state_id src=g.size();
        state_to_id::insert_return_type already=
            id_of_state.insert(d,src);
        if (!already.second)
            return already.first->second; // return id if state was previously explored
        // new state = src - already added by insert above
//        add(id_of_state,d,src); // NOTE: very important that we've added this before we start taking self-epsilons.
        g.push_back();
//        DBPC2("deriving",d);
#if DERIVPRUNE
        remove.push_back(false);
#endif
        typename wfst_io_index::for_state const&fs=io.st[d.s];
        bool dead=(d!=goal);
        if (add_arcs(io,atab,EPS,EPS,d.i,d.o,fs,src)) dead=false;
        bool useO=d.o<out.size(),useI=d.i<in.size();
//        DBPC4("use O I",d,useO,useI);
        unsigned o1=d.o+1,i1=d.i+1;
        if (useO) {
//          DBP(out[d.o]);
          if (add_arcs(io,atab,EPS,out[d.o],d.i,o1,fs,src)) dead=false;
        }

        if (useI) {
            Sym si=in[d.i];
            if (add_arcs(io,atab,si,EPS,i1,d.o,fs,src)) dead=false;
            if (useO)
                if (add_arcs(io,atab,si,out[d.o],i1,o1,fs,src)) dead=false;
        }
#if DERIVPRUNE
        remove[src]=dead;
#endif
        return src;
    }


    template <class arcs_table>
    bool add_arcs(wfst_io_index const& io,arcs_table const&atab,Sym s_in,Sym s_out,unsigned i_in,unsigned i_out
                  ,typename wfst_io_index::for_state const& fs,unsigned source)
    {
        typedef typename wfst_io_index::for_io for_io;
        bool reachgoal=false;
//        DBPC4("looking for arcs",source,IOPair(s_in,s_out),fs);
        if (for_io const* match=find_second(fs,IOPair(s_in,s_out)))
            for (typename for_io::const_iterator i=match->begin(),e=match->end();i!=e;++i) {
                unsigned id=*i;
                ++global_stats.pre.arcs;
                FSTArc *a=atab[id].arc;
                deriv_state ds(i_in,a->dest,i_out);
//                DBPC3("source ->",source,ds);
                unsigned dst=derive(io,atab,ds);
// note: use g[source] rather than caching the iterator, because recursion may invalidate any previously taken iterator
#if DERIVPRUNE
                if (!remove[dst])
#endif
                { //FIXME: remove array isn't showing reachability to final (goal) state, so disabled.
                    g[source].add_data_as(source,dst,a->weight.getReal(),id); // weight only used by gibbs init em prob
                    reachgoal=true;
                }
            }
        return reachgoal;
    }

    reversed_graph r;
    dynamic_array<unsigned> reverse_order;
    // because of pruning, there is not much gain in visiting from the final state in reversed order.  the reverse of reverse_order is the forward order starting at start (0).  and the reverse of that is a valid order for starting at finish and following reverse arcs (it's just that finish may not be the first thing on the reverse_order, if pruning didn't happen, or if there were cycles

    void make_reverse()
    {
        r.init(g);
    }
    void free_reverse()
    {
        if (!cache_backward)
            r.clear();
    }

    void get_reverse()
    {
        if (!cache_backward)
            make_reverse();
    }

    void make_order()
    {
        reverse_order.reserve(g.size());
        reverse_topo_order rev(graph());
        rev.order_from(make_push_backer(reverse_order),start());
        if (rev.get_n_back_edges() > 0)
            Config::warn()<<"Warning: at least one cycle in derivations for example ("<<rev.get_n_back_edges()<<" back edges).  Forward/backward will miss some paths.\n";
    }

    void free_order()
    {
        if (!cache_backward)
            reverse_order.clear();
    }
    void get_order()
    {
        if (!cache_backward)
            make_order();
    }
};

}


#endif
