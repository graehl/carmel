// (bottom-up) best hypertrees and reachability, using the bottom up [vertex]->[arcs with that tail/rules with that vertex in rhs] index: TailsUpHypergraph
#ifndef TAILS_UP_HYPERGRAPH_HPP
#define TAILS_UP_HYPERGRAPH_HPP

// decoder ring: HD = hyperarc descriptor, VD = vertex descriptor

#include "hypergraph.hpp"

template <class HD>
struct HArcDest  {
    HD harc; // hyperarc with this tail
    unsigned multiplicity; // tail multiplicity
    HArcDest(HD e) : harc(e), multiplicity(1) {}
    GENIO_print
        {
          o << '"';
          harc->print(o);
          o << "\"x"<<multiplicity;
          return GENIOGOOD;
        }
};

template <class charT, class Traits,class S>
std::basic_ostream<charT,Traits>&
operator <<
    (std::basic_ostream<charT,Traits>& os, const HArcDest<S> &arg)
{
  return gen_inserter(os,arg);
}



// g: ref to original hypergraph
// terminal_arcs: those with empty tails (already reachable)
// adj: per-vertex list of HArcDest (hyperarc descriptor in g, # of times this vertex appears in tail/rhs) - built with VertMapFactory
// unique_tails: for each hyperarc in g, gives the number of unique tail vertices (used for bottom-up reachability/best-tree) - built with HyperarcMapFactory
// nested class BestTree and Reach are algorithm objects:
    /* usage:
       BestTree alg(g,mu,pi);
       alg.init_costs();
       for each final (tail) vertex v with final cost f:
       alg(v,f);
       alg.finish();

       or:

       alg.init();
       alg.finish(); // same as above but also sets pi for terminal arcs

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

template <class G,
          class HyperarcMapFactory=property_factory<G,hyperarc_tag_t>,
          class VertMapFactory=property_factory<G,vertex_tag_t>,
          class ContS=VectorS >
struct TailsUpHypergraph {
    typedef TailsUpHypergraph<G,HyperarcMapFactory,VertMapFactory,ContS> Self;
    typedef G graph;

    graph &g;
    VertMapFactory vert_fact;
    HyperarcMapFactory h_fact;

    typedef hypergraph_traits<graph> GT;
    typedef typename GT::hyperarc_descriptor HD;
    typedef typename GT::vertex_descriptor VD;

    typedef HArcDest<HD> ArcDest;
    /*  struct Edge : public W {
        unsigned ntails;
        W &weight() { return *this; }
        };
        typedef fixed_array<Edge> Edges;
    */
    //  typedef dynamic_array<Vertex> Vertices;
    //  Edges edge;
    //  Vertices vertex;

// phony vertex @ #V+1? for empty-tail-set harcs? - would require making vertmapfactory have a spot for null_vertex as well ... decided to have separate terminal_arcs list.
    typedef typename ContS::template container<HD>::type TerminalArcs;
    TerminalArcs terminal_arcs;

    /*  typedef typename HyperarcMapFactory::rebind<HD> BestTerminalFactory;
        typedef typename BestTerminalFactory::implementation BestTerminalMap;
        typedef typename BestTerminalFactory::reference BestTerminalPMap;
        BestTerminalMap best_term;
        BestTerminalPMap unique_tails_pmap() {
        return RemainPMap(unique_tails);
        }*/

    typedef typename ContS::template container<ArcDest>::type Adj;
    //typedef fixed_array<Adj> Adjs;
    typedef typename VertMapFactory::template rebind<Adj>::implementation Adjs;
/// index of arcs having a tail
    Adjs adj;
    typedef typename HyperarcMapFactory::template rebind<unsigned> TailsFactory;
    typedef typename TailsFactory::implementation HyperarcLeftMap;
    typedef typename TailsFactory::reference RemainPMap;
    HyperarcLeftMap unique_tails;
    RemainPMap unique_tails_pmap() {
      return ref(unique_tails);
    }


    TailsUpHypergraph(graph& g_,
                      VertMapFactory vert_fact_,
                      HyperarcMapFactory h_fact_) : g(g_), vert_fact(vert_fact_), h_fact(h_fact_), adj(vert_fact), unique_tails(h_fact), terminal_arcs()
                     //num_hyperarcs(g_)
        {
          visit_all();
        }
    TailsUpHypergraph(graph& g_) : g(g_), vert_fact(VertMapFactory(g)), h_fact(HyperarcMapFactory(g)), adj(vert_fact), unique_tails(h_fact), terminal_arcs()
                     //num_hyperarcs(g_)
        {
          visit_all();
        }
    void visit_all() {
      visit(hyperarc_tag,g,ref(*this));
    }
    Adj &operator[](VD v) {
      return adj[v];
    }
    void operator()(HD harc) {
      typename GT::pair_tail_it pti=tails(harc,g);
      if (pti.first==pti.second) {
          terminal_arcs.push(harc);
      } else {
          unsigned ut=0;
          do {
              //Adj &a=adj[get(vi,target(*(pti.first),g))]; // adjacency list for tail
              Adj &a=adj[target(*(pti.first),g)];
              if (a.size()&&a.top().harc == harc) {
                  // last hyperarc with same tail = same hyperarc
                  ++a.top().multiplicity;
              } else {          // new (unique) tail
                  a.push(harc); // default multiplicity=1
                  ++ut;
              }
              ++pti.first;
          } while (pti.first!=pti.second);
          put(unique_tails,harc,ut);
      }
    }

    // caller must init to 0
    template <class EdgePMap>
    void count_unique_tails(EdgePMap e) {
      FOREACH(const Adj &a,adj) {
          FOREACH(const ArcDest &ad,a) {
              ++e[ad.harc];
          }
      }
    }

    // NONNEGATIVE COSTS ONLY!  otherwise may try to adjust the cost of something on heap that was already popped (memory corruption ensues)
    // costmap must be initialized to initial costs (for start vertices) or infinity (otherwise) by user
    // pi (predecessor map) must also be initialized (to hypergraph_traits<G>::null_hyperarc()?) if you want to detect unreached vertices ... although mu=infty can do as well
    // edgecostmap should be initialized to edge costs
    template <
        class VertexCostMap=typename VertMapFactory::template rebind<typename property_map<graph,edge_weight_t>::cost_type>::reference,
        //class VertexPredMap=property_factory<graph,VD>::template rebind<HD>::reference
        class VertexPredMap=typename VertMapFactory::template rebind<HD>::reference,
        //dummy_property_map

        class EdgeCostMap=property_map<graph,edge_weight_t>
        >
    struct BestTree {
        typedef typename VertMapFactory::template rebind<HD>::implementation DefaultPi;
        typedef typename VertMapFactory::template rebind<typename property_map<graph,edge_weight_t>::cost_type>::implementation DefaultMu;
        Self &rev;
        VertexCostMap mu;
        VertexPredMap pi;
        typedef typename VertMapFactory::template rebind<void *> LocFact;
        typedef typename LocFact::implementation Locs;


        Locs loc;

        //typedef typename unwrap_reference<VertexCostMap>::type::value_type Cost;
        typedef typename unwrap_reference<EdgeCostMap>::type::value_type Cost;
        struct RemainInf : public std::pair<unsigned,Cost> {
            unsigned remain() const { return this->first; }
            Cost cost() const { return this->second; }

            unsigned & remain() { return this->first; }
            Cost & cost() { return this->second; }
            GENIO_print {
              o << "(" << remain() << "," << cost() << ")";
              return GENIOGOOD;
            }
        };
        typedef typename HyperarcMapFactory::template rebind<RemainInf> RemainInfCostFact; // lower bound on edge costs
        typedef typename RemainInfCostFact::implementation RemainInfCosts;
        RemainInfCosts remain_infinum;
        typename RemainInfCostFact::reference hyperarc_remain_and_cost_map() {
          return ref(remain_infinum);
        }

        //TailsRemainMap tr;
        //HyperarcLeftMap tr;

        typedef HeapKey<VD,VertexCostMap,typename LocFact::reference> Key;
        typedef dynamic_array<Key> Heap;
        Heap heap;

        BestTree(Self &r,VertexCostMap mu_,VertexPredMap pi_=VertexPredMap())
            :
            rev(r),
            mu(mu_),
            pi(pi_),
            loc(rev.vert_fact),
            remain_infinum(rev.h_fact),
            heap(num_vertices(rev.g))
            {
              visit(hyperarc_tag,
                    rev.g,
                    make_indexed_pair_copier(ref(remain_infinum),rev.unique_tails_pmap(),get(edge_weight,r.g))); // pair(rem)<-(tr,ev)
            }
        // semi-tricky: loc should be default initialized (void *) to 0
        BestTree(Self &r,VertexCostMap mu_,VertexPredMap pi_, EdgeCostMap ec)
            :
            rev(r),
            mu(mu_),
            pi(pi_),
            loc(rev.vert_fact),
            remain_infinum(rev.h_fact),
            heap(num_vertices(rev.g))
            {
              visit(hyperarc_tag,
                    rev.g,
                    make_indexed_pair_copier(ref(remain_infinum),rev.unique_tails_pmap(),ec)); // pair(rem)<-(tr,ev)
            }


        static Cost infinity() {
          return numeric_limits<Cost>::infinity();
        }
        void init_costs(Cost cinit=infinity()) {
          typename GT::pair_vertex_it i=vertices(rev.g);
          for (;i.first!=i.second;++i.first) {
              put(mu,*i.first,cinit);
          }
        }
        Cost get_inf(HD h) const {
          return get(remain_infinum,h).cost();
        }
        void init() {
          init_costs();
          for (typename TerminalArcs::iterator i=rev.terminal_arcs.begin(),end=rev.terminal_arcs.end();i!=end;++i) {
              HD h=*i;
              VD v=source(h,rev.g);
              Cost hc=get_inf(h);
              //        typename unwrap_reference<VertexCostMap>::type &dmu(mu);
              Cost &mc=deref(mu)[v];
              if (hc < mc) {
                  mc=hc;
                  put(pi,v,h);
                  safe_queue(v);
              }
          }
        }
        void operator()(VD v,Cost c) {
          put(mu,v,c);
          //typename Key::SetLocWeight save(ref(loc),mu);
          heap.push_back(v);
        }
    private:
        void operator()(VD v) {
          if (get(mu,v) != infinity()) {
              Assert(!was_queued(v));
              heap.push_back(v);
          }
        }
    public:
        void queue_all() {
          //typename Key::SetLocWeight save(ref(loc),mu);
          visit(vertex_tag,rev.g,ref(*this));
        }
        template<class I>
        void queue(I begin, I end) {
          //typename Key::SetLocWeight save(ref(loc),mu);
          std::for_each(begin,end,ref(*this));
        }
        void safe_queue(VD v) {
          if (!was_queued(v))
              heap.push_back(v);
        }
        bool was_queued(VD v) {
          return get(loc,v) != NULL;
        }
        void relax(VD v,HD h,Cost c) {
          Cost &m=deref(mu)[v];
          if (c < m) {
              m=c;
              heapAdjustOrAdd(heap,Key(v));
              put(pi,v,h);
          }
        }
        void reach(VD v) {
          Cost cv=get(mu,v);
          const Adj &a=rev[v];
          FOREACH(const ArcDest &ad,a) { // for each hyperarc v participates in as a tail
              HD h=ad.harc;
              VD head=source(h,rev.g);
              RemainInf &ri=remain_infinum[h];
              Assert(ri.cost() >= 0);
              ri.cost() += (ad.multiplicity*cv);  // assess the cost of reaching v
              Assert(ri.remain() > 0);
              if (--ri.remain() == 0) // if v completes the hyperarc, attempt to use it to reach head (more cheaply)
                  relax(head,h,ri.cost());
          }

        }
        void finish() {
          typename Key::SetLocWeight save(ref(loc),mu);
          heapBuild(heap);
          while(heap.size()) {
              VD top=heap.front().key;
              heapPop(heap);
              reach(top);
          }
        }
        void go() 
        {
            init();
            finish();
        }

    };


    // reachmap must be initialized to false by user
    template <
        class VertexReachMap=typename VertMapFactory::template rebind<bool>::reference
        >
    struct Reach {
        typedef typename VertMapFactory::template rebind<bool>::implementation DefaultReach;
        Self &rev;
        VertexReachMap vr;
        HyperarcLeftMap tr;
        unsigned n;
        Reach(Self &r,VertexReachMap v) : rev(r),vr(v),tr(r.unique_tails),n(0) {
          //copy_hyperarc_pmap(rev.g,rev.unique_tails_pmap(),tr);
          //instead relying on implementation same type (copy ctor)
        }

        void init_unreach() {
          typename GT::pair_vertex_it i=vertices(rev.g);
          for (;i.first!=i.second;++i.first) {
              put(vr,*i.first,false);
              //        deref(vr)[*i.first]=false;
              //        put(deref(vr),*i.first,false);
          }
        }
        void init() {
          init_unreach();
          for (typename TerminalArcs::iterator i=rev.terminal_arcs.begin(),end=rev.terminal_arcs.end();i!=end;++i) {
              HD h=*i;
              VD v=source(h,rev.g);
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
          Adj &a=rev[v];
          for(typename Adj::iterator i=a.begin(),end=a.end(); i!=end; ++i) {
              HD harc=i->harc;
              VD head=source(harc,rev.g);
              if (!get(vr,head)) { // not reached yet
                  if (--tr[harc]==0)
                      (*this)(head); // reach head
              }
          }

        }
        RemainPMap tails_remain_pmap() {
          return ref(tr);
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
      std::for_each(begin,end,ref(alg));
      return alg.n;
    }





};

//! TESTS in ../tt/transducergraph.hpp


#endif
