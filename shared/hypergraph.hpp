#ifndef _HYPERGRAPH_HPP
#define _HYPERGRAPH_HPP

// adjacency lists (rules or var+rule leaving state) - should be able to use boost dijkstra
// reverse var+rule index allowing djikstra-ish best-tree and completely-derivable detection

/*
  using Boost Graph Library

  view a transducer or other ordered multi-hypergraph as:

  directed graph of outside edges (from each tail to head, i.e. rhs states to lhs state)


  G       A type that is a model of Graph.
  g       An object of type G.
  v       An object of type boost::graph_traits<G>::vertex_descriptor.
  Associated Types
  boost::graph_traits<G>::traversal_category

  This tag type must be convertible to adjacency_graph_tag.

  boost::graph_traits<G>::adjacency_iterator

  An adjacency iterator for a vertex v provides access to the vertices adjacent to v. As such, the value type of an adjacency iterator is the vertex descriptor type of its graph. An adjacency iterator must meet the requirements of MultiPassInputIterator.
  Valid Expressions
  adjacent_vertices(v, g)         Returns an iterator-range providing access to the vertices adjacent to vertex v in graph g.[1]
  Return type: std::pair<adjacency_iterator, adjacency_iterator>


  http://www.boost.org/libs/graph/doc/adjacency_list.html

*/


#include "ttconfig.hpp"
#include "property.hpp"
#include "transducer.hpp"
#include "list.h"
#include "dynarray.h"
#include "weight.h"
//#include "byref.hpp"
#include <boost/ref.hpp>
#include <boost/graph/graph_traits.hpp>
#include "adjustableheap.hpp"

#include "graph.hpp"


template <class T>
struct hypergraph_traits {
  typedef T graph;
  typedef graph_traits<graph> GT;
  typedef typename graph::hyperarc_descriptor hyperarc_descriptor;
  typedef typename graph::hyperarc_iterator hyperarc_iterator;
  typedef typename graph::tail_descriptor tail_descriptor;
  typedef typename graph::tail_iterator tail_iterator;

  //  typedef typename graph::hyperarc_index_map hyperarc_index_map;

  typedef std::pair<
    tail_iterator,
    tail_iterator> pair_tail_it;
  typedef std::pair<
    hyperarc_iterator,
    hyperarc_iterator> pair_hyperarc_it;

  /*
    typedef typename graph::out_hyperarc_iterator out_hyperarc_iterator;
    typedef std::pair<
    out_hyperarc_iterator,
    out_hyperarc_iterator> pair_out_hyperarc_it;
  */

  typedef typename GT::vertex_descriptor      vertex_descriptor;
  typedef typename GT::edge_descriptor        edge_descriptor;
  typedef typename GT::adjacency_iterator     adjacency_iterator;
  typedef typename GT::out_edge_iterator      out_edge_iterator;
  typedef typename GT::in_edge_iterator       in_edge_iterator;
  typedef typename GT::vertex_iterator        vertex_iterator;
  typedef typename GT::edge_iterator          edge_iterator;

  typedef typename GT::directed_category      directed_category;
  typedef typename GT::edge_parallel_category edge_parallel_category;
  typedef typename GT::traversal_category     traversal_category;

  typedef typename GT::vertices_size_type     vertices_size_type;
  typedef typename GT::edges_size_type        edges_size_type;
  typedef typename GT::degree_size_type       degree_size_type;

  typedef std::pair<
    vertex_iterator,
    vertex_iterator> pair_vertex_it;
  typedef std::pair<
    edge_iterator,
    edge_iterator> pair_edge_it;

};

struct hyperarc_tag_t {
};

static hyperarc_tag_t hyperarc_tag;

template <class G> struct graph_object<G,hyperarc_tag_t> {
  typedef typename hypergraph_traits<G>::hyperarc_descriptor descriptor;
  typedef typename hypergraph_traits<G>::hyperarc_iterator iterator;
  typedef std::pair<iterator,iterator> iterator_pair;
};

/*
  struct NoWeight {
  void setOne() {}
  };
*/

// use boost::iterator_property_map<RandomAccessIterator, OffsetMap, T, R> with OffsetMap doing the index mapping
// usually: K = key *, you have array of key at key *: vec ... vec+size
// construct OffsetArrayPmap(vec,vec+size) and get an array of size Vs (default constructed)

/*Iterator      Must be a model of Random Access Iterator.
  OffsetMap       Must be a model of Readable Property Map and the value type must be convertible to the difference type of the iterator.
  T       The value type of the iterator.         std::iterator_traits<RandomAccessIterator>::value_type
  R       The reference type of the iterator.     std::iterator_traits<RandomAccessIterator>::reference

  iterator_property_map(Iterator i, OffsetMap m)*/



/*template <class V,class O>
  struct ArrayPMap;
*/




/*: public ByRef<ArrayPMapImp<V,O> >
  {
  typedef ArrayPMapImp<V,O> Imp;
  typedef typename Imp::category category;
  typedef typename Imp::key_type key_type;
  typedef typename Imp::value_type value_type;
  explicit ArrayPMap(Imp &a) : ByRef<Imp>(a) {}
  };
*/

// HyperarcLeftMap = count of unique tails for each edge, should be initialized to 0 by user
// e.g.
/*
  typedef typename graph_traits<G>::hyperarc_index_map HaIndex;
  typedef ArrayPMap<unsigned,HaIndex> PMap;
  typename PMap::Imp arc_remain(num_hyperarcs(g),HaIndex(g));
  ReverseHypergraph<G,PMap> r(g,PMap(arc_remain));
*/

template <class G,class P1,class P2>
void copy_hyperarc_pmap(G &g,P1 a,P2 b) {
  visit(hyperarc_tag,make_indexed_copier(a,b));
}



template <class G,
          //class HyperarcLeftMap=typename ArrayPMap<unsigned,typename graph_traits<G>::hyperarc_index_map>::type,
  class HyperarcMapFactory=property_factory<G,hyperarc_tag_t>,
          class VertMapFactory=property_factory<G,vertex_tag_t>,
          class ContS=VectorS >

// phony vertex @ #V+1? for empty-tail-set harcs? - would require making vertmapfactory have a spot for null_vertex as well
struct ReverseHyperGraph {
  typedef ReverseHyperGraph<G,HyperarcMapFactory,VertMapFactory,ContS> Self;
  typedef G graph;

  graph &g;
  VertMapFactory vert_fact;
  HyperarcMapFactory h_fact;

  typedef hypergraph_traits<graph> GT;
  typedef typename GT::hyperarc_descriptor HD;
  typedef typename GT::vertex_descriptor VD;

  struct ArcDest  {
    HD harc; // hyperarc with this tail
    unsigned multiplicity; // tail multiplicity
    ArcDest(HD e) : harc(e), multiplicity(1) {}
    GENIO_print_on
    {
      harc.print_on(o);
      o << " multiplicity="<<multiplicity;
      return GENIOGOOD;
    }
  };

  /*  struct Edge : public W {
      unsigned ntails;
      W &weight() { return *this; }
      };
      typedef FixedArray<Edge> Edges;
  */
  //  typedef DynamicArray<Vertex> Vertices;
  //  Edges edge;
  //  Vertices vertex;

  typedef typename ContS::container<HD>::type TerminalArcs;
  TerminalArcs terminal_arcs;

  /*  typedef typename HyperarcMapFactory::rebind<HD> BestTerminalFactory;
      typedef typename BestTerminalFactory::implementation BestTerminalMap;
      typedef typename BestTerminalFactory::reference BestTerminalPMap;
      BestTerminalMap best_term;
      BestTerminalPMap unique_tails_pmap() {
      return RemainPMap(unique_tails);
      }*/

  typedef typename ContS::container<ArcDest>::type Adj;
  //typedef FixedArray<Adj> Adjs;
  typedef typename VertMapFactory::rebind<Adj>::implementation Adjs;
  Adjs adj;
  typedef typename HyperarcMapFactory::rebind<unsigned> TailsFactory;
  typedef typename TailsFactory::implementation HyperarcLeftMap;
  typedef typename TailsFactory::reference RemainPMap;
  HyperarcLeftMap unique_tails;
  RemainPMap unique_tails_pmap() {
    return RemainPMap(unique_tails);
  }


  ReverseHyperGraph(graph& g_,
                    VertMapFactory vert_fact_,
                    HyperarcMapFactory h_fact_) : g(g_), vert_fact(vert_fact_), h_fact(h_fact_), adj(vert_fact), unique_tails(h_fact), terminal_arcs()
    //num_hyperarcs(g_)
  {
   visit_all();
  }
  ReverseHyperGraph(graph& g_) : g(g_), vert_fact(VertMapFactory(g)), h_fact(HyperarcMapFactory(g)), adj(vert_fact), unique_tails(h_fact), terminal_arcs()
    //num_hyperarcs(g_)
  {
   visit_all();
  }
  void visit_all() {
     visit(hyperarc_tag,g,*this);
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

    // user must init to 0
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
    class VertexCostMap=typename VertMapFactory::rebind<typename property_map<graph,edge_weight_t>::cost_type>::reference,
    //class VertexPredMap=property_factory<graph,VD>::rebind<HD>::reference
    class VertexPredMap=typename VertMapFactory::rebind<HD>::reference,
      //dummy_property_map

    class EdgeCostMap=property_map<graph,edge_weight_t>
  >
  struct BestTree {
    typedef typename VertMapFactory::rebind<HD>::implementation DefaultPi;
    typedef typename VertMapFactory::rebind<typename property_map<graph,edge_weight_t>::cost_type>::implementation DefaultMu;
    Self &rev;
    VertexCostMap mu;
    VertexPredMap pi;
    typedef typename VertMapFactory::rebind<void *> LocFact;
    typedef typename LocFact::implementation Locs;


    Locs loc;

    //typedef typename unwrap_reference<VertexCostMap>::type::value_type Cost;
    typedef typename unwrap_reference<EdgeCostMap>::type::value_type Cost;
    struct RemainInf : public std::pair<unsigned,Cost> {
      unsigned & remain() { return first; }
      Cost & cost() { return second; }
    };
    typedef typename HyperarcMapFactory::rebind<RemainInf> RemainInfCostFact; // lower bound on edge costs
    typedef typename RemainInfCostFact::implementation RemainInfCosts;
    RemainInfCosts remain_infinum;
    typename RemainInfCostFact::reference hyperarc_remain_and_cost_map() {
      return remain_infinum;
    }

    //TailsRemainMap tr;
    //HyperarcLeftMap tr;

    typedef HeapKey<VD,VertexCostMap,typename LocFact::reference> Key;
    typedef DynamicArray<Key> Heap;
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
      std::foreach(begin,end,ref(*this));
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
  };


  // reachmap must be initialized to false by user
  template <
    class VertexReachMap=typename VertMapFactory::rebind<bool>::reference
  >
  struct Reach {
    typedef typename VertMapFactory::rebind<bool>::implementation DefaultReach;
    Self &rev;
    VertexReachMap vr;
    HyperarcLeftMap tr;
    unsigned n;
    Reach(Self &r,VertexReachMap v) : rev(r),vr(v),tr(r.unique_tails),n(0) {
      //copy_hyperarc_pmap(rev.g,rev.unique_tails_pmap(),tr);
      //relying on implementation same type (copy ctor)
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
      return RemainPMap(tr);
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


  /* usage:
     BestTree alg(g,mu,pi);
     alg.init_costs();
     for each final (tail) vertex v with final cost f:
     alg(v,f);
     alg.finish();

     or:

     alg.init();
     alg.finish(); // same as above but also sets pi for terminal arcs

     or ... assign to mu final costs yourself and then
     alg.queue_all(); // adds non-infinity cost only
     alg.finish();

     also
     typename RemainInfCostFact::reference hyperarc_remain_and_cost_map()
     returns pmap with pmap[hyperarc].remain() == 0 if the arc was usable from the final tail set
     and pmap[hyperarc].cost() being the cheapest cost to reaching all final tails
  */




};

//! TESTS in transducergraph.hpp

#endif
