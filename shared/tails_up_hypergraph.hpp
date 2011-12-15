#ifndef GRAEHL_SHARED__TAILS_UP_HYPERGRAPH_HPP
#define GRAEHL_SHARED__TAILS_UP_HYPERGRAPH_HPP

// (bottom-up) best hypertrees and reachability, using the bottom up [vertex]->[arcs with that tail/rules with that vertex in rhs] index: TailsUpHypergraph

// g: ref to original hypergraph
// terminal_arcs: those with empty tails (already reachable)
// adj: per-vertex list of HArcDest (hyperarc descriptor in g, # of times this vertex appears in tail/rhs) - built with VertMapFactory
// unique_tails: for each hyperarc in g, gives the number of unique tail vertices (used for bottom-up reachability/best-tree) - built with HyperarcMapFactory
// nested class BestTree and Reach are algorithm objects:

/* usage:
   typedef TailsUpHypergraph<H> T;
   T t(g);
   T::template BestTree<> alg(t,mu,pi);
   for each final (tail) vertex v with final cost f:
   alg.axiom(v,f);
   alg.finish();

   or:

   alg.init(); // same as above but also sets pi for terminal arcs
   alg.finish();

   or:

   alg.go(); // does init() and finish()

   or ... assign to mu final costs yourself and then
   alg.queue_all(); // adds non-infinity cost only
   alg.finish();

   also
   typename RemainInfCostFact::reference hyperarc_remain_and_cost_map()
   returns pmap with pmap[hyperarc].remain() == 0 if the arc was usable from the final tail set
   and pmap[hyperarc].cost() being the cheapest cost to reaching all final tails
*/

#define DEBUG_TAILS_UP_HYPERGRAPH 1

#if DEBUG_TAILS_UP_HYPERGRAPH
# define TUHG(x) x
#else
# define TUHG(x)
#endif

//slows down debug mode.
#define TUHG_CHECK_HEAP 0

#if TUHG_CHECK_HEAP
#define DEBUG_D_ARY_HEAP 1
#define TUHG_EXTRA_Q TUHG_EXTRA_QALL("heap")
#define DEFAULT_DBG_D_ARY_VERIFY_HEAP 0
#else
#define TUHG_EXTRA_Q
#endif

#include <graehl/shared/os.hpp>
#include <graehl/shared/show.hpp>
#include <graehl/shared/containers.hpp>
#include <graehl/shared/property_factory.hpp>
#include <graehl/shared/hypergraph.hpp>
#include <graehl/shared/print_read.hpp>
#include <graehl/shared/word_spacer.hpp>
# include <graehl/shared/d_ary_heap.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/type_traits/remove_reference.hpp>
//#include <boost/ref.hpp>


namespace graehl {
DECLARE_DBG_LEVEL(TUHG)
//IFDBG(TUHG,1) { SHOWM2(TUHG,"descr" }
#define TUHG_EXTRA_Q1(n) SHOWM1(TUHG,n,print(heap,range_sep()));
#define TUHG_EXTRA_Q2(n) SHOWM1(TUHG,n,print(heap,pair_getter(mu)));
#define TUHG_EXTRA_QALL(n) TUHG_EXTRA_Q1(n) TUHG_EXTRA_Q2(n)
#define TUHG_SHOWQ(l,n,v) EIFDBG(TUHG,l,SHOWM3(TUHG,n,v,mu[v],heap.loc(v));TUHG_EXTRA_Q )
#define TUHG_SHOWP(l,n,mu) EIFDBG(TUHG,l,SHOWM(TUHG,n,print(vertices(g),pair_getter(mu))))

struct BestTreeStats {
  std::size_t n_blocked_rereach,n_relax,n_update,n_pop,n_unpopped;
  BestTreeStats() : n_blocked_rereach(),n_relax(),n_update(),n_pop(),n_unpopped() {}
  typedef BestTreeStats self_type;
  template <class O>
  void print(O &o) const {
    o<<"BestTreeStats:"
     <<" blocked "<<n_blocked_rereach<<" negative-cost improvements"
     <<"; evaluated "<<n_relax<<" edges"
      <<" and improved "<<n_update<<" of those"
      <<"; found best cost of "<<n_pop<<" vertices"
      <<" and left "<<n_unpopped<<" reachable vertices unused"
     << ".";
  }
  TO_OSTREAM_PRINT
};

/* we're going to store these indexed by tail vertex for the purpose of bottom-up reachability (topo-sort-like)
   // changed my mind: don't need multiplicity; we can just visit original graphs' tails in order and count/repeat if we want. just track unique ED instead
 */
template <class ED>
struct HTailMult  {
  ED ed; // hyperarc with this tail
  unsigned multiplicity; // tail multiplicity - usually 1
  HTailMult(ED e) : ed(e), multiplicity(1) {}
  typedef HTailMult<ED> self_type;
  template <class O> void print(O&o) const {
    o<<'"';
    ed->print(o);
    o<<'"';
    o << "\"x"<<multiplicity;
  }
  TO_OSTREAM_PRINT
};

// stores G ref
template <class G,
          class EdgeMapFactory=property_factory<G,edge_tag>,
          class VertMapFactory=property_factory<G,vertex_tag>,
          class ContS=VectorS // how we hold the adjacent edges for tail vert.
>
struct TailsUpHypergraph {
  //ED: hyperedge descriptor. VD: vertex
  typedef TailsUpHypergraph<G,EdgeMapFactory,VertMapFactory,ContS> Self;
  typedef G graph;

  graph const&g;
  VertMapFactory vert_fact;
  EdgeMapFactory edge_fact;

  typedef boost::graph_traits<graph> GT;
  typedef graehl::edge_traits<graph> ET;
  typedef graehl::path_traits<graph> PT;
  typedef typename ET::tail_iterator Ti;
  typedef std::pair<Ti,Ti> Tailr;
  typedef typename GT::edge_descriptor ED;
  typedef typename GT::vertex_descriptor VD;

// typedef HTailMult<ED> TailMult;
  typedef ED Tail;

  typedef typename ContS::template container<ED>::type TerminalArcs;
  TerminalArcs terminal_arcs;

  typedef typename ContS::template container<Tail>::type Adj;
  typedef typename VertMapFactory::template rebind<Adj>::impl Adjs;
  Adjs adj;
  typedef typename EdgeMapFactory::template rebind<unsigned> TailsFactory;
  typedef typename TailsFactory::impl EdgeLeftImpl;
  typedef typename TailsFactory::reference EdgeLeftMap;
  EdgeLeftImpl unique_tails;
  EdgeLeftMap unique_tails_pmap;


  TailsUpHypergraph(graph const& g_,
                    VertMapFactory vert_fact_,
                    EdgeMapFactory edge_fact_)
    : g(g_)
    , vert_fact(vert_fact_)
    , edge_fact(edge_fact_)
    , adj(vert_fact.template init<Adj>())
    , unique_tails(edge_fact.template init<unsigned>())
    , unique_tails_pmap(unique_tails)
    , terminal_arcs()
  {
    record_edges();
  }
  TailsUpHypergraph(graph const& g_)
    : g(g_)
    ,vert_fact(VertMapFactory(g))
    , edge_fact(EdgeMapFactory(g))
    , adj(vert_fact.template init<Adj>())
    , unique_tails(edge_fact.template init<unsigned>())
    , unique_tails_pmap(unique_tails)
    , terminal_arcs()
  {
    record_edges();
  }
  void record_edges() {
    visit(edgeT,g,*this);
  }
  Adj &operator[](VD v) {
    return adj[v];
  }
  void operator()(ED ed) {
    Tailr pti=tails(ed,g);
    if (pti.first==pti.second) {
      terminal_arcs.push_back(ed);
    } else {
      unsigned ntails_uniq=0;
      do {
        //Adj &a=adj[get(vi,target(*(pti.first),g))]; // adjacency list for tail
        Adj &a=adj[tail(*pti.first,ed,g)];
        Tail const&la=last_added(a);
        if (a.size()&&la == ed) {
          // last hyperarc with same tail = same hyperarc
// ++la.multiplicity;
        } else { // new (unique) tail
          add(a,Tail(ed)); // default multiplicity=1
          ++ntails_uniq;
        }
        ++pti.first;
      } while (pti.first!=pti.second);
      put(unique_tails_pmap,ed,ntails_uniq);
    }
  }

  // caller must init to 0
  template <class EdgePMap>
  void count_unique_tails(EdgePMap &e) {
    for(typename Adjs::const_iterator i=adj.begin(),e=adj.end();i!=e;++i) {
      for (typename Adj::const_iterator j=i->begin(),ej=i->end();j!=ej;++j) {
// const TailMult &ad=*j;
        ++e[*j];
      }
    }
  }

  typedef std::size_t heap_loc_t;

  typedef typename PT::cost_type cost_type;
  // NONNEGATIVE COSTS ONLY! otherwise may try to adjust the cost of something on heap that was already popped (memory corruption ensues)
  // costmap must be initialized to initial costs (for start vertices) or infinity (otherwise) by user
  // pi (predecessor map) must also be initialized (to hypergraph_traits<G>::null_hyperarc()?) if you want to detect unreached vertices ... although mu=infty can do as well
  // edgecostmap should be initialized to edge costs
  template <
    class VertexCostMap=typename VertMapFactory::template rebind<cost_type>::reference,
    //class VertexPredMap=property_factory<graph,VD>::template rebind<ED>::reference
    class VertexPredMap=typename VertMapFactory::template rebind<ED>::reference,
    //dummy_property_map
    class EdgeCostMap=typename EdgeMapFactory::template rebind<cost_type>::reference
  >
  struct BestTree {
// typedef typename VertMapFactory::template rebind<ED>::impl DefaultPi;
// typedef typename VertMapFactory::template rebind<cost_type>::impl DefaultMu;
    Self &tu;
    graph const& g;

    unsigned allow_rereach; // allow repeated requeueing of already "best known" popped nodes, this many times (0 means don't need to track, otherwise we instantiate rereachptr). we count to avoid infinite loop if neg cost cycle. this count isn't used until we requeue something we popped; we don't mind if we find new best costs for a tail repeatedly as long as we didn't pop the head of that edge yet.

    VertexCostMap mu;
    VertexPredMap pi;
    typedef typename VertMapFactory::template rebind<heap_loc_t> LocFact;
    typedef typename LocFact::impl Locs;
    typedef typename LocFact::reference LocsP;

    Locs loc;
    LocsP locp;

    //typedef typename boost::unwrap_reference<VertexCostMap>::type::value_type Cost;
// typedef typename boost::unwrap_reference<EdgeCostMap>::type::value_type Cost;
// typedef typename boost::property_traits<EdgeCostMap>::value_type Cost;
    typedef cost_type Cost;
    typedef typename ET::tails_size_type Ntails;

    struct RemainInf : public std::pair<Ntails,Cost> {
      typedef std::pair<Ntails,Cost> P;
      RemainInf() : P(0,PT::start()) {}
      RemainInf(Ntails const& n,Cost const& c) : P(n,c) {}
      Ntails remain() const { return this->first; }
      Cost const& cost() const { return this->second; }
// operator Cost() const { return this->second; }
// typedef Cost value_type;
      Ntails & remain() { return this->first; }
      Cost & cost() { return this->second; }
      template <class O> void print(O&o) const {
        o << "(" << remain() << "," << cost() << ")";
      }
      typedef RemainInf self_type;
      TO_OSTREAM_PRINT
    };

    typedef typename EdgeMapFactory::template rebind<Ntails> RemainTailsFact; // lower bound on edge costs
    typedef typename RemainTailsFact::impl RemainTails;
    RemainTails remain;
    typedef typename RemainTailsFact::reference RemainTailsPmap;
    RemainTailsPmap remain_pmap;
    EdgeCostMap ec;

    typedef built_pmap<vertex_tag,graph,unsigned> Rereach;
    typedef boost::shared_ptr<Rereach> RereachPtr;
    typedef typename Rereach::property_map_type RereachP;
    RereachPtr rereachptr;
    RereachP rereach;


    unsigned tail_already_reached(VD v) const {
      return allow_rereach && get(rereach,v);
    }

    unsigned already_reached(VD v) const {
      return allow_rereach ? get(rereach,v) : get(locp,v)==(heap_loc_t)D_ARY_HEAP_NULL_INDEX;
      // if we're not allow_rereach tracking, then this is only meaningful for heads, not the tail (which was just popped in every case)
    }
    void mark_reached(VD v) {
      if (allow_rereach)
        ++rereach[v];
#ifdef NDEBUG
      else // we don't use locp for anything if allow_rereach. but this pretties up the debug output
#endif
        put(locp,v,(heap_loc_t)D_ARY_HEAP_NULL_INDEX); // not possible to have this before being added first time - i cleared it to 0s in building form vert_fact (default construct)
    }

    BestTreeStats stat;
    typedef d_ary_heap_indirect<VD,graehl::OPTIMAL_HEAP_ARITY,LocsP,VertexCostMap,updates_cost<graph> > Heap;
    Heap heap;
    BestTree(Self &r,VertexCostMap mu_,VertexPredMap pi_, EdgeCostMap ec,unsigned allow_rereach=0)
      :
      tu(r)
      , g(tu.g)
      , allow_rereach(allow_rereach)
      , mu(mu_)
      , pi(pi_)
      , loc(tu.vert_fact.template init<heap_loc_t>())
      , locp(loc)
      , remain(tu.edge_fact.template init<Ntails>())
      , remain_pmap(remain)
      , ec(ec)
      , rereachptr(allow_rereach?RereachPtr(new Rereach(g,0)):RereachPtr())
      , rereach(allow_rereach?rereachptr->pmap:RereachP())
      , heap(mu,locp)
    {
      copy_pmap(edgeT,g,remain_pmap,tu.unique_tails_pmap);
// visit(edgeT,g,make_indexed_pair_copier(remain_pmap,tu.unique_tails_pmap,ec)); // pair(rem)<-(tr,ev)
      init_costs();
    }

    void init_pi(ED null=ED()) {
      graehl::init_pmap(vertexT,g,pi,null);
    }

    void init_costs(Cost cinit=PT::unreachable()) {
      typename GT::vertex_iterator i,end;
      boost::tie(i,end)=vertices(g);
      for (;i!=end;++i) {
        VD v=*i;
        put(locp,v,0); // I'm only doing this because I can't tell if new int[100] calls int(). it should
        put(mu,*i,cinit);
      }
    }
    void init() { // fill from hg terminal arcs
      for (typename TerminalArcs::iterator i=tu.terminal_arcs.begin(),end=tu.terminal_arcs.end();i!=end;++i) {
        ED h=*i;
        VD v=source(h,g);
        Cost hc=get(ec,h);
        // typename unwrap_reference<VertexCostMap>::type &dmu(mu);
        axiom(v,hc);
      }
    }

    void axiom(VD axiom,Cost const& c=PT::start(),ED h=ED()) {
      EIFDBG(TUHG,3,SHOW2(TUHG,axiom,print(axiom,g)));
      Cost &mc=mu[axiom];
      if (PT::update(c,mc)) {
        safe_queue(axiom);
        put(pi,axiom,h);
      }
    }

    void operator()(VD v,Cost const& c) {
      axiom(v,c);
    }

    void add_unsorted(VD v) { // call finish() after
      heap.add_unsorted(v);
      TUHG_SHOWQ(1,"added_unsorted",v);
    }

    struct add_axioms {
      BestTree &b;
      explicit add_axioms(BestTree &b) : b(b) {}
      void operator()(VD v) const {
        b.axiom(v);
      }
    };

    add_axioms axiom_adder() {
      return add_axioms(*this);
    }

    void operator()(VD v) { // can be called blindly for all verts. only those with reachable path cost are queued
      //put(locp,v,0); // not necessary: property factory (even new int[N] will always default init
      if (!is_queued(v) && get(mu,v) != PT::unreachable()) {
        add_unsorted(v);
      }
    }

    // must have no duplicates, and have already set mu
    void queue_all() {
      visit(vertexT,g,*this);
    }
    void safe_queue(VD v) {
      TUHG_SHOWQ(2,"safe_queue",v);
      if (!is_queued(v))
        add_unsorted(v);
    }

    bool is_queued(VD v) const {
// return get(loc,v) || (!heap.empty() && heap.top()==v); // 0 init relied upon, but 0 is a valid location. could set locs to -1 beforehand instead
      return heap.contains(v);
    }
    VD top() const {
      return heap.top();
    }
    void pop() {
      ++stat.n_pop;
      TUHG_SHOWQ(1,"pop",heap.top());
// IFDBG(TUHG,1) { SHOWM(TUHG,"pop",heap.top()); }
      heap.pop();
    }

    void relax(VD head,ED e,Cost const& c) {
      ++stat.n_relax;
      Cost &m=mu[head];
      EIFDBG(TUHG,3,SHOWM5(TUHG,"relax?",head,print(head,g),c,mu[head],print(e,g)));
      if (PT::update(c,m)) {
        ++stat.n_update;
        put(pi,head,e);
        IFDBG(TUHG,2) { SHOWM4(TUHG,"updating-or-adding",head,print(head,g),mu[head],heap.loc(head)); }
        heap.push_or_update(head);
// IFDBG(TUHG,3) { SHOWM3(TUHG,"updated-or-added",head,print(head,g),heap.loc(head)); }
        TUHG_SHOWQ(3,"updated-or-added",head);
      }
    }

    //skipping unreached tails:
    cost_type recompute_cost(ED e) {
      cost_type c=get(ec,e);
      IFDBG(TUHG,3) { SHOWM2(TUHG,"recompute_cost base",c,print(e,g)); }
      for (Tailr pti=tails(e,g);pti.first!=pti.second;++pti.first) {
        VD t=*pti.first;
        cost_type tc=get(mu,t);
        if (tc!=PT::unreachable())
          c=PT::extend(c,tc); // possibly multiple
        IFDBG(TUHG,6) { SHOWM3(TUHG,"recompute_cost tail",t,tc,c); }
      }
      IFDBG(TUHG,3) { SHOWM2(TUHG,"recompute_cost final",c,print(e,g)); }
      return c;
    }

    void reach(VD tail) {
      TUHG_SHOWQ(2,"reach",tail);
      const Adj &a=tu[tail];
// FOREACH(const TailMult &ad,a) { // for each hyperarc v participates in as a tail
      bool tail_already=tail_already_reached(tail);
      mark_reached(tail);
      IFDBG(TUHG,2) { SHOWM4(TUHG,"reach",tail,print(tail,g),mu[tail],tail_already); }
      for (typename Adj::const_iterator j=a.begin(),ej=a.end();j!=ej;++j) {
        ED e=*j;
        VD head=target(e,g);
        IFDBG(TUHG,4) { SHOWM3(TUHG,"satisfying",tail,head,print(e,g)); }
        if (already_reached(head) > allow_rereach) { // we don't know reach count of head
// don't even propagate improved costs, because we can't otherwise guarantee no infinite loop.
          ++stat.n_blocked_rereach;
          IFDBG(TUHG,1) { SHOWM3(TUHG,"blocked",head,already_reached(head),stat.n_blocked_rereach); }
        } else {
          EIFDBG(TUHG,4,SHOWM3(TUHG,"may yet reach",head,already_reached(head),print(e,g)));
          /* for negative costs: will need to track every tails' cost last used for an edge, or just compute edge cost from scratch every time. or need to remember for each vertex last cost used. */
// const TailMult &ad=*j;
// ri.cost() = PT::extend(ri.cost(),PT::repeat(get(mu,tail),ad.multiplicity)); // assess the cost of reaching v // only reason to do this early is to have a bound and discard edge forever if head head_already better-reached. doesn't seem important to do so. TODO: remove cost() member
          // if v completes the hyperarc, or we allow rereaching, attempt to use it to reach head (more cheaply):
          Ntails &tails_unreached=remain_pmap[e];
          if (!tail_already) {
            assert(tails_unreached > 0);
            --tails_unreached;
            EIFDBG(TUHG,4,SHOWM4(TUHG,"Decreased tails_unreached",tail,e,tails_unreached,print(e,g)));
          }
          if (!tails_unreached)
            relax(head,e,recompute_cost(e)); // may re-relax if already reached. may infinite loop if we allow re-reaching
          //TODO: incremental recompute cost due to just improving tail v (need to store last weight used for v before improvement)
        }
      }
    }

#define TUHG_SHOWREMAIN(l,n) IFDBG(TUHG,l) {SHOWP(TUHG,"\n" n " (remain):\n");dbgremain();SHOWNL(TUHG);}
    //IFDBG(TUHG,l) { SHOWP(TUHG,n " (remain):\n");dbgremain();SHOWNL(TUHG); }

    void dbgremain() const {
      visit(edgeT,g,*this);
    }
    void operator()(ED e) const {
      SHOW3(TUHG,e,remain_pmap[e],print(e,g));
    }

    void finish() {
#define TUHG_SHOWP_ALL(l,n) TUHG_SHOWP(l,n,mu); TUHG_SHOWP(l,n,pi); if (allow_rereach) { TUHG_SHOWP(l,n,rereach); } TUHG_SHOWREMAIN(l,n);
      TUHG_SHOWP(1,"pre-heapify",locp);
      EIFDBG(TUHG,5,TUHG_EXTRA_QALL("pre-heapify"));
      heap.heapify();
      EIFDBG(TUHG,4,TUHG_EXTRA_QALL("post-heapify"));
      TUHG_SHOWP(1,"post-heapify",locp);
      TUHG_SHOWP_ALL(1,"pre-finish");
      while(!heap.empty()) {
        VD top=this->top();
        TUHG_SHOWQ(6,"at-top",top);
        EIFDBG(TUHG,5,SHOW3(TUHG,heap.size(),top,print(top,g)));
        pop();
        reach(top);
        TUHG_SHOWP_ALL(9,"post-reach");
      }
      stat.n_unpopped=heap.size();
      TUHG_SHOWP_ALL(5,"post-finish");
      EIFDBG(TUHG,1,SHOW(TUHG,stat));
    }
    void go()
    {
      init();
      finish();
    }
  };


#if 0
  //TODO:
  // reachmap must be initialized to false by user
  template <
    class VertexReachMap=typename VertMapFactory::template rebind<bool>::reference
  >
  struct Reach {
    typedef typename VertMapFactory::template rebind<bool>::impl DefaultReach;
    Self &tu;
    VertexReachMap vr;
    EdgeLeftImpl tr;
    unsigned n;
    Reach(Self &r,VertexReachMap v) : tu(r),vr(v),tr(r.unique_tails),n(0) {
      //copy_hyperarc_pmap(g,tu.unique_tails_pmap(),tr);
      //instead relying on impl same type (copy ctor)
    }

    void init_unreach() {
      typename GT::vertex_iterator i,end;
      boost::tie(i,end)==vertices(g);
      for (;i!=end;++i) {
        put(vr,*i.first,false);
      }
    }
    void init() {
      init_unreach();
      for (typename TerminalArcs::iterator i=tu.terminal_arcs.begin(),end=tu.terminal_arcs.end();i!=end;++i) {
        ED h=*i;
        VD v=source(h,g);
        (*this)(v);
      }
    }
    void finish() {
    }
    void go()
    {
      init();
      finish();
    }
    void operator()(VD v) {
      if (get(vr,v))
        return;
      ++n;
      put(vr,v,true); // mark reached
      Adj &a=tu[v];
      for(typename Adj::iterator i=a.begin(),end=a.end(); i!=end; ++i) {
        ED ed=i->ed;
        VD head=source(ed,g);
        if (!get(vr,head)) { // not reached yet
          if (--tr[ed]==0)
            (*this)(head); // reach head
        }
      }

    }
    EdgeLeftMap tails_remain_pmap() {
      return EdgeLeftMap(tr);
    }

  };

  template <class P>
  unsigned reach(VD start,P p) {
    Reach<P> alg(*this,p);
    alg(start);
    return alg.n;
  }
  template <class B,class E,class VertexReachMap>
  unsigned reach(B begin,E end,VertexReachMap p) {
    Reach<VertexReachMap> alg(*this,p);
    std::for_each(begin,end,boost::ref(alg));
    return alg.n;
  }
#endif
};

}//graehl

#endif
