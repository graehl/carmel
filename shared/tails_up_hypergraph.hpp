// Copyright 2014 Jonathan Graehl - http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/** \file

  (bottom-up) best hypertrees and reachability, using the bottom up [vertex]->[arcs with that tail/rules with
  that vertex in rhs] index: TailsUpHypergraph. Useful for lazy-kbest and useless-state/arc pruning

// g: ref to original hypergraph
// terminal_arcs: those with empty tails (already reachable)
// adj: per-vertex list of HArcDest (hyperarc descriptor in g, # of times this vertex appears in tail/rhs) -
// built with VertMapFactory
// unique_tails: for each hyperarc in g, gives the number of unique tail vertices (used for bottom-up
// reachability/best-tree) - built with HyperarcMapFactory
// nested class BestTree and Reach are algorithm objects:

usage:
   typedef TailsUpHypergraph<H> T;
   T t(g);
   T::template BestTree<> alg(t, mu, pi);
   for each final (tail) vertex v with final cost f:
   alg.axiom(v, f);
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

   TODO: option to stop once you reach goal vertex (this is all-destinations right now)
*/

#ifndef GRAEHL_SHARED__TAILS_UP_HYPERGRAPH_HPP
#define GRAEHL_SHARED__TAILS_UP_HYPERGRAPH_HPP
#pragma once


#ifndef GRAEHL_DEBUG_TAILS_UP_HYPERGRAPH
#define GRAEHL_DEBUG_TAILS_UP_HYPERGRAPH 0
#endif
#include <graehl/shared/ifdbg.hpp>
#if GRAEHL_DEBUG_TAILS_UP_HYPERGRAPH
#define TUHG(x) x
#include <graehl/shared/show.hpp>
#define TUHG_SHOWREMAIN(l, n)
#define TUHG_SHOWP_ALL(l, n)
#else
#define TUHG(x)
#define TUHG_DBG_LEVEL -100
#endif


#define TUHG_PRINT(a, b) print(a, b)

// leave 0 or you will greatly slow the *debug* build (release will run fine)
#define TUHG_CHECK_HEAP 0

#if TUHG_CHECK_HEAP
#define GRAEHL_DEBUG_D_ARY_HEAP 1
#define TUHG_EXTRA_Q() TUHG_EXTRA_QALL("heap")
#define DEFAULT_DBG_D_ARY_VERIFY_HEAP 0
#else
#define TUHG_EXTRA_Q()
#endif

#define TUHG_EXTRA_Q1(n) SHOWIF1(TUHG, 1, n, TUHG_PRINT(heap, range_sep()));
#define TUHG_EXTRA_Q2(n) SHOWIF1(TUHG, 2, n, TUHG_PRINT(heap, pair_getter(mu)));
#define TUHG_EXTRA_QALL(n) TUHG_EXTRA_Q1(n) TUHG_EXTRA_Q2(n)
#define TUHG_SHOWQ(l, n, v)                   \
  SHOWIF3(TUHG, l, n, v, mu[v], heap.loc(v)); \
  TUHG_EXTRA_Q()
#define TUHG_SHOWP(l, n, mu) SHOWIF1(TUHG, l, n, TUHG_PRINT(vertices(g), pair_getter(mu)))

#include <graehl/shared/os.hpp>
#include <graehl/shared/containers.hpp>
#include <graehl/shared/hypergraph.hpp>
#include <graehl/shared/property_factory.hpp>
#include <graehl/shared/print_read.hpp>
#include <graehl/shared/word_spacer.hpp>
#include <graehl/shared/d_ary_heap.hpp>
#include <graehl/shared/string_to.hpp>
#include <graehl/shared/verbose_exception.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>


namespace graehl {

VERBOSE_EXCEPTION_DECLARE(BestTreeRereachException)

#if GRAEHL_DEBUG_TAILS_UP_HYPERGRAPH
DECLARE_DBG_LEVEL(TUHG)
#endif

struct BestTreeStats {
  std::size_t n_blocked_rereach, n_relax, n_update, n_converged_inexact, n_pop, n_unpopped;
  BestTreeStats()
      : n_blocked_rereach(), n_relax(), n_update(), n_converged_inexact(), n_pop(), n_unpopped() {}
  typedef BestTreeStats self_type;
  template <class O>
  void print(O& o) const {
    o << "BestTreeStats:"
      << " blocked " << n_blocked_rereach << " negative-cost improvements"
      << "; evaluated " << n_relax << " edges"
      << " and improved " << n_update << " of those";
    if (n_converged_inexact)
      o << ", skipping re-queueing of " << n_converged_inexact << " by within-epsilon convergence";
    o << "; found best cost of " << n_pop << " vertices";
    if (n_unpopped) o << " and left " << n_unpopped << " reachable vertices unused";
    o << ".";
  }
  TO_OSTREAM_PRINT
};

struct BestTreeOptions {
  unsigned allow_rereach;  // allow repeated requeueing of already "best known" popped nodes, this many times
  // (0 means don't need to track, otherwise we instantiate rereachptr). we count to
  // avoid infinite loop if neg cost cycle. this count isn't used until we requeue
  // something we popped; we don't mind if we find new best costs for a tail
  // repeatedly as long as we didn't pop the head of that edge yet.
  bool throw_on_rereach_limit;  // throw exception on excess of allow_rereach if true, otherwise just
  // increment stat.n_blocked_rereach
  std::string convergence_epsilon_str;  // if set, stop before allow_rereach limit if
  // PT::converged(improvement, previous, convergence_epsilon) - that
  // is, plus(improvement, previous)<convergence_epsilon.
  BestTreeOptions() { defaults(); }
  void defaults() {
    allow_rereach = 0;
    throw_on_rereach_limit = false;
    convergence_epsilon_str
        = "";  // this is a string because path_traits may dictate something other than float
  }
  template <class Conf>
  void configure(Conf& c) {
    c.is("Best tree");
    c("rereach", &allow_rereach)(
        "mostly for handling lattices with net-negative-cost edges: in best-first allow the 'best' path to a "
        "node to be reached this many times (may be needed if edge costs are negative more than sum of best "
        "paths to all-but-one tail is positive). if every edge has a net-postivie cost, this can be 0, which "
        "saves memory");
    c("throw-on-max-rereach", &throw_on_rereach_limit)(
        "if a node is popped more than rereach+1 times, throw a BestTreeRereachException immediately)");
    c("convergence", &convergence_epsilon_str)(
        "if empty or 0, only stop on maximum # of rereaches or 0 change anywhere. else stop if change is "
        "below this amount")
        .is("epsilon - a small nonnegative real");
  }
};

/* we're going to store these indexed by tail vertex for the purpose of bottom-up reachability
(topo-sort-like)
// changed my mind: don't need multiplicity; we can just visit original graphs' tails in order and
count/repeat if we want. just track unique ED instead
*/
template <class ED>
struct HTailMult {
  ED ed;  // hyperarc with this tail
  unsigned multiplicity;  // tail multiplicity - usually 1
  HTailMult(ED e) : ed(e), multiplicity(1) {}
  typedef HTailMult<ED> self_type;
  template <class O>
  void print(O& o) const {
    o << '"';
    ed->print(o);
    o << '"';
    o << "\"x" << multiplicity;
  }
  TO_OSTREAM_PRINT
};

// stores G ref
template <class G, class EdgeMapFactory = property_factory<G, edge_tag>,
          class VertMapFactory = property_factory<G, vertex_tag>,
          class ContS = VectorS  // how we hold the adjacent edges for tail vert.
          >
struct TailsUpHypergraph {
  // ED: hyperedge descriptor. VD: vertex
  typedef TailsUpHypergraph<G, EdgeMapFactory, VertMapFactory, ContS> Self;
  typedef G graph;

  graph const& g;
  VertMapFactory vert_fact;
  EdgeMapFactory edge_fact;

  typedef boost::graph_traits<graph> GT;
  typedef graehl::edge_traits<graph> ET;
  typedef graehl::path_traits<graph> PT;
  typedef typename ET::tail_iterator Ti;
  typedef boost::iterator_range<Ti> Tailr;
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


  TailsUpHypergraph(graph const& g_, VertMapFactory vert_fact_, EdgeMapFactory edge_fact_)
      : g(g_)
      , vert_fact(vert_fact_)
      , edge_fact(edge_fact_)
      , adj(vert_fact.template init<Adj>())
      , unique_tails(edge_fact.template init<unsigned>())
      , unique_tails_pmap(unique_tails)
      , terminal_arcs() {
    record_edges();
  }
  TailsUpHypergraph(graph const& g_)
      : g(g_)
      , vert_fact(VertMapFactory(g))
      , edge_fact(EdgeMapFactory(g))
      , adj(vert_fact.template init<Adj>())
      , unique_tails(edge_fact.template init<unsigned>())
      , unique_tails_pmap(unique_tails)
      , terminal_arcs() {
    record_edges();
  }
  void record_edges() { visit(edgeT, g, *this); }
  Adj& operator[](VD v) { return adj[v]; }
  void operator()(ED ed) {
    Tailr tailr = tails(ed, g);
    Ti i = boost::begin(tailr), e = boost::end(tailr);
    if (i == e) {
      terminal_arcs.push_back(ed);
    } else {
      unsigned ntails_uniq = 0;
      do {
        Adj& a = adj[tail(*i, ed, g)];
        if (a.size() && last_added(a) == ed) {
          // last hyperarc with same tail = same hyperarc
        } else {  // new (unique) tail
          add(a, Tail(ed));  // default multiplicity=1
          ++ntails_uniq;
        }
        ++i;
      } while (i != e);
      put(unique_tails_pmap, ed, ntails_uniq);
    }
  }

  // caller must init to 0
  template <class EdgePMap>
  void count_unique_tails(EdgePMap& e) {
    for (typename Adjs::const_iterator i = adj.begin(), e = adj.end(); i != e; ++i) {
      for (typename Adj::const_iterator j = i->begin(), ej = i->end(); j != ej; ++j) {
        // TailMult const& ad=*j;
        ++e[*j];
      }
    }
  }

  typedef std::size_t heap_loc_t;

  typedef typename PT::cost_type cost_type;

  struct BestTreeOptionsParsed : BestTreeOptions {
    bool use_convergence;
    cost_type convergence_epsilon;
    // bool update_predecessor_on_blocked_rereach; // right now this is hardcoded true.
    BestTreeOptionsParsed() { defaults(); }
    BestTreeOptionsParsed(BestTreeOptions const& opt) : BestTreeOptions(opt) { parse(); }
    void defaults() {
      BestTreeOptions::defaults();
      parse();
    }
    // call this after updating base.
    void parse() {
      use_convergence = !convergence_epsilon_str.empty();
      if (use_convergence) {
        string_to(convergence_epsilon_str, convergence_epsilon);
        if (convergence_epsilon == PT::start()) use_convergence = false;
      }
    }
  };

  // costmap must be initialized to initial costs (for start vertices) or infinity (otherwise) by user
  // pi (predecessor map) must also be initialized (to hypergraph_traits<G>::null_hyperarc()?) if you want to
  // detect unreached vertices ... although mu=infty can do as well
  // edgecostmap should be initialized to edge costs
  template <class VertexCostMap = typename VertMapFactory::template rebind<cost_type>::reference,
            // class VertexPredMap=property_factory<graph, VD>::template rebind<ED>::reference
            class VertexPredMap = typename VertMapFactory::template rebind<ED>::reference,
            // dummy_property_map
            class EdgeCostMap = typename EdgeMapFactory::template rebind<cost_type>::reference>
  struct BestTree {
    // typedef typename VertMapFactory::template rebind<ED>::impl DefaultPi;
    // typedef typename VertMapFactory::template rebind<cost_type>::impl DefaultMu;
    Self& tu;
    graph const& g;
    BestTreeOptionsParsed opt;

    VertexCostMap mu;
    VertexPredMap pi;
    typedef typename VertMapFactory::template rebind<heap_loc_t> LocFact;
    typedef typename LocFact::impl Locs;
    typedef typename LocFact::reference LocsP;

    Locs loc;
    LocsP locp;

    // typedef typename boost::unwrap_reference<VertexCostMap>::type::value_type Cost;
    // typedef typename boost::unwrap_reference<EdgeCostMap>::type::value_type Cost;
    // typedef typename boost::property_traits<EdgeCostMap>::value_type Cost;
    typedef cost_type Cost;

    typedef typename ET::tails_size_type Ntails;

    struct RemainInf : public std::pair<Ntails, Cost> {
      typedef std::pair<Ntails, Cost> P;
      RemainInf() : P(0, PT::start()) {}
      RemainInf(Ntails const& n, Cost const& c) : P(n, c) {}
      Ntails remain() const { return this->first; }
      Cost const& cost() const { return this->second; }
      // operator Cost() const { return this->second; }
      // typedef Cost value_type;
      Ntails& remain() { return this->first; }
      Cost& cost() { return this->second; }
      template <class O>
      void print(O& o) const {
        o << "(" << remain() << "," << cost() << ")";
      }
      typedef RemainInf self_type;
      TO_OSTREAM_PRINT
    };

    typedef typename EdgeMapFactory::template rebind<Ntails> RemainTailsFact;  // lower bound on edge costs
    typedef typename RemainTailsFact::impl RemainTails;
    RemainTails remain;
    typedef typename RemainTailsFact::reference RemainTailsPmap;
    RemainTailsPmap remain_pmap;
    EdgeCostMap ec;

    typedef built_pmap<vertex_tag, graph, unsigned> Rereach;
    typedef shared_ptr<Rereach> RereachPtr;
    typedef typename Rereach::property_map_type RereachP;
    RereachPtr rereachptr;
    RereachP rereach;

    unsigned tail_already_reached(VD v) const { return opt.allow_rereach && get(rereach, v); }


    unsigned already_reached(VD v) const {
      return opt.allow_rereach ? get(rereach, v) : get(locp, v) == (heap_loc_t)GRAEHL_D_ARY_HEAP_NULL_INDEX;
      // if we're not allow_rereach tracking, then this is only meaningful for
      // heads, not the tail (which was just popped in every case)
    }
    void mark_reached(VD v) {
      if (opt.allow_rereach) ++rereach[v];
#ifdef NDEBUG
      else  // we don't use locp for anything if allow_rereach. but this pretties up the debug output
#endif
        put(locp, v, (heap_loc_t)GRAEHL_D_ARY_HEAP_NULL_INDEX);
      // not possible to have this before being added first time - i cleared it
      // to 0s in building form vert_fact (default construct)
    }

    BestTreeStats stat;
    typedef d_ary_heap_indirect<VD, graehl::OPTIMAL_HEAP_ARITY, VertexCostMap, LocsP, better_cost<graph> > Heap;
    Heap heap;
    BestTree(Self& r, VertexCostMap mu_, VertexPredMap pi_, EdgeCostMap ec,
             BestTreeOptionsParsed const& bestOpt = BestTreeOptions())
        : tu(r)
        , g(tu.g)
        , opt(bestOpt)
        , mu(mu_)
        , pi(pi_)
        , loc(tu.vert_fact.template init<heap_loc_t>())
        , locp(loc)
        , remain(tu.edge_fact.template init<Ntails>())
        , remain_pmap(remain)
        , ec(ec)
        , rereachptr(opt.allow_rereach ? RereachPtr(new Rereach(g, 0)) : RereachPtr())
        , rereach(opt.allow_rereach ? rereachptr->pmap : RereachP())
        , heap(mu, locp) {
      opt.parse();
      copy_pmap(edgeT, g, remain_pmap, tu.unique_tails_pmap);
      // visit(edgeT, g, make_indexed_pair_copier(remain_pmap, tu.unique_tails_pmap, ec)); // pair(rem)<-(tr,
      // ev)
      init_costs();
    }

    void init_pi(ED null = ED()) {
      if (pi) graehl::init_pmap(vertexT, g, pi, null);
    }

    typedef typename GT::vertex_iterator Vi;
    typedef boost::iterator_range<Vi> Vertices;
    void init_costs(Cost cinit = PT::unreachable()) {
      Vertices verts = vertices(g);
      for (Vi i = boost::begin(verts), e = boost::end(verts); i != e; ++i) {
        VD v = *i;
        put(mu, *i, cinit);
        put(locp, v, 0);
        // this should not be necessary for shared_array_property_map (new int[]
        // default-inits), but it turns out that it actually is necessary. anyway we should support all pmaps
        assert(get(locp, v) == 0);
      }
    }
    void init() {  // fill from hg terminal arcs
      for (typename TerminalArcs::iterator i = tu.terminal_arcs.begin(), end = tu.terminal_arcs.end();
           i != end; ++i) {
        ED h = *i;
        VD v = source(h, g);
        Cost hc = get(ec, h);
        // typename unwrap_reference<VertexCostMap>::type &dmu(mu);
        axiom(v, hc);
      }
    }

    void safe_queue(VD v) {
      TUHG_SHOWQ(2, "safe_queue", v);
      if (!is_queued(v)) add_unsorted(v);
    }

    void axiom(VD axiom, Cost const& c = PT::start(), ED h = ED()) {
      SHOWIF2(TUHG, 3, c, axiom, TUHG_PRINT(axiom, g));
      Cost& mc = mu[axiom];
      if (PT::update(c, mc)) {
        assert(PT::includes(c, mc));
        safe_queue(axiom);
        if (pi) put(pi, axiom, h);
      } else {
        SHOWIF4(TUHG, 0, "WARNING: axiom didn't improve mu[axiom]", c, axiom, mu[axiom], TUHG_PRINT(axiom, g));
      }
    }

    void operator()(VD v, Cost const& c) { axiom(v, c); }

    void add_unsorted(VD v) {  // call finish() after
      heap.add_unsorted(v);
      TUHG_SHOWQ(1, "added_unsorted", v);
    }

    struct add_axioms {
      BestTree& b;
      explicit add_axioms(BestTree& b) : b(b) {}
      void operator()(VD v) const { b.axiom(v); }
    };

    add_axioms axiom_adder() { return add_axioms(*this); }

    void
    operator()(VD v) {  // can be called blindly for all verts. only those with reachable path cost are queued
      // put(locp, v,0); // not necessary: property factory (even new int[N] will always default init
      if (!is_queued(v) && get(mu, v) != PT::unreachable()) {
        add_unsorted(v);
      }
    }

    // must have no duplicates, and have already set mu
    void queue_all() { visit(vertexT, g, *this); }

    bool is_queued(VD v) const {
      // return get(loc, v) || (!heap.empty() && heap.top()==v); // 0 init relied upon, but 0 is a valid
      // location. could set locs to -1 beforehand instead
      return heap.contains(v);
    }
    VD top() const { return heap.top(); }
    void pop() {
      ++stat.n_pop;
      TUHG_SHOWQ(1, "pop", heap.top());
      heap.pop();
    }

    void relax(VD head, ED e, Cost const& c) {
      ++stat.n_relax;
      Cost& m = mu[head];
      Cost mu_prev = m;
      SHOWIF5(TUHG, 3, "relax?", head, TUHG_PRINT(head, g), c, mu[head], TUHG_PRINT(e, g));
      if (PT::update(c, m)) {
        if (pi) put(pi, head, e);
        assert(PT::includes(c, m));
        if (opt.use_convergence && PT::converged(c, mu_prev, opt.convergence_epsilon)) {
          SHOWIF5(TUHG, 2, "converged", head, TUHG_PRINT(head, g), mu_prev, mu[head], opt.convergence_epsilon);
          ++stat.n_converged_inexact;
        } else {
          ++stat.n_update;
          SHOWIF5(TUHG, 2, "updating-or-adding", head, TUHG_PRINT(head, g), mu_prev, mu[head], heap.loc(head));
          heap.push_or_update(head);
        }
        TUHG_SHOWQ(3, "relaxed", head);
      } else {
        SHOWIF5(TUHG, 3, "no improvement", head, TUHG_PRINT(head, g), c, mu[head], TUHG_PRINT(e, g));
      }
    }

    // skipping unreached tails:
    cost_type recompute_cost(ED e) {
      cost_type c = get(ec, e);
      SHOWIF2(TUHG, 3, "recompute_cost:base", c, TUHG_PRINT(e, g));
      Tailr tailr = tails(e, g);
      for (Ti i = boost::begin(tailr), ei = boost::end(tailr); i != ei; ++i) {
        VD t = tail(*i, e, g);  // possibly multiple instances of tail t
        cost_type tc = get(mu, t);
        SHOWIF3(TUHG, 6, "recompute_cost:c'=c*tc", c, tc, t);
        assert(tc != PT::unreachable());  // FIXME: maybe we want to allow this (used to be "if")? if we
        // don't, then you can only reach with non-unreachable cost.
        PT::extendBy(tc, c);
      }
      SHOWIF2(TUHG, 3, "recompute_cost:final", c, TUHG_PRINT(e, g));
      return c;
    }

    void reach(VD tail) {
      TUHG_SHOWQ(2, "reach", tail);
      Adj const& a = tu[tail];
      // FOREACH(TailMult const& ad, a) { // for each hyperarc v participates in as a tail
      bool tail_already = tail_already_reached(tail);
      mark_reached(tail);
      SHOWIF4(TUHG, 2, "reach", tail, TUHG_PRINT(tail, g), mu[tail], tail_already);
      for (typename Adj::const_iterator j = a.begin(), ej = a.end(); j != ej; ++j) {
        ED e = *j;
        VD head = target(e, g);
        SHOWIF3(TUHG, 4, "satisfying", tail, head, TUHG_PRINT(e, g));
        if (already_reached(head) > opt.allow_rereach) {  // we don't know reach count of head
          // don't even propagate improved costs, because we can't otherwise guarantee no infinite loop.
          ++stat.n_blocked_rereach;
          SHOWIF3(TUHG, 1, "blocked", head, already_reached(head), stat.n_blocked_rereach);
          if (opt.throw_on_rereach_limit)
            VTHROW_A_3(BestTreeRereachException,
                       "Exceeded rereach limit (improving an already-would-be-best-without-negative-costs "
                       "path-tree for a vertex). If increasing rereach limit doesn't stop this, you may have "
                       "a negative-cost cycle (so no best tree).",
                       already_reached(head), opt.allow_rereach);
        } else {
          SHOWIF3(TUHG, 4, "may yet reach", head, already_reached(head), TUHG_PRINT(e, g));
          /* for negative costs: will need to track every tails' cost last used for an edge, or just compute
           * edge cost from scratch every time. or need to remember for each vertex last cost used. */
          // TailMult const& ad=*j;
          // ri.cost() = PT::extend(ri.cost(), PT::repeat(get(mu, tail), ad.multiplicity)); // assess the cost
          // of reaching v // only reason to do this early is to have a bound and discard edge forever if head
          // head_already better-reached. doesn't seem important to do so. TODO: remove cost() member
          // if v completes the hyperarc, or we allow rereaching, attempt to use it to reach head (more
          // cheaply):
          Ntails& tails_unreached = remain_pmap[e];
          if (!tail_already) {
            assert(tails_unreached > 0);
            --tails_unreached;
            SHOWIF4(TUHG, 4, "Decreased tails_unreached", tail, e, tails_unreached, TUHG_PRINT(e, g));
          }
          if (!tails_unreached)
            relax(head, e,
                  recompute_cost(
                      e));  // may re-relax if already reached. may infinite loop if we allow re-reaching
          // TODO: incremental recompute cost due to just improving tail v (need to store last weight used for
          // v before improvement)
        }
      }
    }

#if GRAEHL_DEBUG_TAILS_UP_HYPERGRAPH
    void dbgremain() const { visit(edgeT, g, *this); }
    void operator()(ED e) const { SHOW3(TUHG, e, remain_pmap[e], TUHG_PRINT(e, g)); }
#define TUHG_SHOWREMAIN(l, n)           \
  IFDBG(TUHG, l) {                      \
    SHOWP(TUHG, "\n" n " (remain):\n"); \
    dbgremain();                        \
    SHOWNL(TUHG);                       \
  }
#define TUHG_SHOWP_ALL(l, n)   \
  TUHG_SHOWP(l, n, mu);        \
  TUHG_SHOWP(l, n, pi);        \
  if (opt.allow_rereach) {     \
    TUHG_SHOWP(l, n, rereach); \
  }                            \
  TUHG_SHOWREMAIN(l, n);
#else
#define TUHG_SHOWREMAIN(l, n)
#define TUHG_SHOWP_ALL(l, n)
#endif


    void finish() {
      // TUHG_SHOWP(1, "pre-heapify", locp);
      heap.heapify();
      // TUHG_SHOWP(1, "post-heapify", locp);
      TUHG_SHOWP_ALL(1, "pre-finish");
      while (!heap.empty()) {
        VD top = this->top();
        TUHG_SHOWQ(6, "at-top", top);
        SHOWIF2(TUHG, 5, heap.size(), top, TUHG_PRINT(top, g));
        pop();
        reach(top);
        TUHG_SHOWP_ALL(9, "post-reach");
      }
      stat.n_unpopped = heap.size();
      TUHG_SHOWP_ALL(5, "post-finish");
      SHOWIF0(TUHG, 1, stat);
    }
    void go() {
      init();
      finish();
    }
  };
};


}

#endif
