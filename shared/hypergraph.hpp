#ifndef _HYPERGRAPH_HPP
#define _HYPERGRAPH_HPP

// adjacency lists (rules or var+rule leaving state) - should be able to use boost dijkstra
// reverse var+rule index allowing djikstra-ish best-tree and completely-derivable detection

/*
using Boost Graph Library

view a transducer or other ordered multi-hypergraph as:

directed graph of outside edges (from each tail to head, i.e. rhs states to lhs state)


G 	A type that is a model of Graph.
g 	An object of type G.
v 	An object of type boost::graph_traits<G>::vertex_descriptor.
Associated Types
boost::graph_traits<G>::traversal_category

This tag type must be convertible to adjacency_graph_tag.

boost::graph_traits<G>::adjacency_iterator

An adjacency iterator for a vertex v provides access to the vertices adjacent to v. As such, the value type of an adjacency iterator is the vertex descriptor type of its graph. An adjacency iterator must meet the requirements of MultiPassInputIterator.
Valid Expressions
adjacent_vertices(v, g) 	Returns an iterator-range providing access to the vertices adjacent to vertex v in graph g.[1]
Return type: std::pair<adjacency_iterator, adjacency_iterator>


http://www.boost.org/libs/graph/doc/adjacency_list.html

*/


#include "ttconfig.hpp"
#include "graph.hpp"
#include "property.hpp"
#include "transducer.hpp"
#include "list.h"
#include "dynarray.h"
#include "weight.h"
//#include "byref.hpp"
#include <boost/ref.hpp>
#include <boost/graph/graph_traits.hpp>
#include "adjustableheap.hpp"



/*
struct NoWeight {
  void setOne() {}
};
*/

// use boost::iterator_property_map<RandomAccessIterator, OffsetMap, T, R> with OffsetMap doing the index mapping
// usually: K = key *, you have array of key at key *: vec ... vec+size
// construct OffsetArrayPmap(vec,vec+size) and get an array of size Vs (default constructed)

/*Iterator 	Must be a model of Random Access Iterator. 	 
OffsetMap 	Must be a model of Readable Property Map and the value type must be convertible to the difference type of the iterator. 	 
T 	The value type of the iterator. 	std::iterator_traits<RandomAccessIterator>::value_type
R 	The reference type of the iterator. 	std::iterator_traits<RandomAccessIterator>::reference

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
  visit<typename graph_traits<G>::hyperarc_descriptor>(make_indexed_copier(a,b));
}



template <class G,
//class HyperarcLeftMap=typename ArrayPMap<unsigned,typename graph_traits<G>::hyperarc_index_map>::type,
class HyperarcMapFactory=typename property_factory<G,typename graph_traits<G>::hyperarc_descriptor>,
  class VertMapFactory=typename property_factory<G,typename graph_traits<G>::vertex_descriptor>,
class ContS=VectorS >

struct ReverseHyperGraph {
  typedef ReverseHyperGraph<G,HyperarcMapFactory,VertMapFactory,ContS> Self;
  typedef G graph;
  typedef graph_traits<graph> GT;
  typedef typename GT::hyperarc_descriptor HD;    
  typedef typename GT::vertex_descriptor VD;    

  struct ArcDest  {
	HD harc; // hyperarc with this tail
	unsigned multiplicity; // tail multiplicity
	ArcDest(HD e) : harc(e), multiplicity(1) {}
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
  
  typedef typename ContS::container<ArcDest>::type Adj;
  //typedef FixedArray<Adj> Adjs;
  typedef typename VertMapFactory::rebind<Adj>::implementation Adjs;
  graph &g;
  Adjs adj;
  typedef typename HyperarcMapFactory::rebind<unsigned> TailsFactory;
  typedef typename TailsFactory::implementation HyperarcLeftMap;
  typedef typename TailsFactory::reference RemainPMap;
  HyperarcMapFactory h_fact;
  HyperarcLeftMap unique_tails;  
  RemainPMap unique_tails_pmap() {
    return RemainPMap(unique_tails);
  }
  VertMapFactory vert_fact;
  
  
  ReverseHyperGraph(const G& g_,
    VertMapFactory vert_fact_=VertMapFactory(g_),
    HyperarcMapFactory h_fact_=HyperarcMapFactory(g_)) : adj(vert_fact_), g(g_), unique_tails(unique_tails_pmap), vert_fact(vert_fact_), h_fact(h_fact_), unique_tails(h_fact_)
    //num_hyperarcs(g_)
  {		
	visit<HD>(g,*this);
  }
  
  Adj &operator[](VD v) {
    return adj[v];
  }
  void operator()(HD harc) {
    unsigned ut=0;
    for (typename graph_traits<graph>::pair_tail_iterator pti=tails(harc,g);
      pti.first!=pti.second; ++pti.first) {
        //Adj &a=adj[get(vi,target(*(pti.first),g))]; // adjacency list for tail
        Adj &a=adj[target(*(pti.first),g)];
        if (a.size()&&a.top().harc == harc) {
          // last hyperarc with same tail = same hyperarc
	      ++a.top().multiplicity;
        } else {	  // new (unique) tail
	      a.push(harc); // default multiplicity=1
	      ++ut; 
	    }
      }
    put(unique_tails,harc,ut);
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
 // edgecostmap should be initialized to edge costs
 template <
   class VertexCostMap=typename property_factory<G,VD>::rebind<float>::reference,
   //class VertexPredMap=property_factory<G,VD>::rebind<HD>::reference
   class VertexPredMap=dummy_property_map,
   class EdgeCostMap=property_map<G,edge_weight_t>
 >
 struct BestTree {
  BestTree(Self &rev_,VertexCostMap mu_,VertexPredMap pi_=PredMap(),
    EdgeCostMap ec=get(edge_weight,rev_.g))
    :
    rev(rev_),mu(mu_),pi(pi_),loc(rev_.g),remain_infinum(rev_.g),heap(num_vertices(rev_.g)) {
      //copy_hyperarc_pmap(rev.g,rev.tails_remain_pmap(),tr);      //already copied above.
      //copy_hyperarc_pmap(rev.g,ec_,infinum); // can't rely on copying implementation since no API for it
      visit<typename GT::hyperarc_descriptor>(
        make_indexed_pair_copier(ref(remain_infinum),rev.tails_remain_pmap(),ec));
    }   // semi-tricky: loc should be default initialized (void *) to 0

   Self &rev;
  VertexCostMap mu;
  VertexPredMap pi;
  //TailsRemainMap tr;
  HyperarcLeftMap tr;
  typedef typename VertMapFactory::rebind<void *> LocMap;
  typedef typename LocMap::implementation Locs;


  Locs loc;

  typedef typename VertexCostMap::value_type Cost;
  struct RemainInf : public std::pair<unsigned,Cost> {
    unsigned & remain() { return first; }
    Cost & cost() { return second; }
  };
  typedef typename HyperarcMapFactory::rebind<RemainInf> RemainInfCostMap; // lower bound on edge costs
  typedef typename RemainInfCostMap::implementation RemainInfCosts;
  RemainInfCosts remain_infinum;
  typename RemainInfCostMap::reference hyperarc_remain_and_cost_map() {
    return remain_infinum;
  }

  typedef HeapKey<VD,VertexCostMap,typename LocMap::reference> Key;
  typedef DynamicArray<Key> Heap;
  Heap heap;
  

  void init_costs(Cost cinit=numeric_limits<Cost>::infinity) {
    typename GT::pair_vertex_it i=vertices(rev.g);
    for (;i.first!=i.second;++i.first) {
      put(mu,i.first,cinit);
    }
  }
  void operator()(VD v,Cost c) {
    put(mu,v,c);
    //typename Key::SetLocWeight save(ref(loc),mu);
    heap.push_back(v);
  }
 private:      
  void operator()(VD v) {    
    if (get(mu,v) != numeric_limits<Cost>::infinity)
      heap.push_back(v);
  }
 public:
  void prepare() {
    //typename Key::SetLocWeight save(ref(loc),mu);
    visit<VD>(rev.g,ref(*this));
  }
  template<class I>
  void prepare(I begin, I end) {
    //typename Key::SetLocWeight save(ref(loc),mu);
    std::foreach(begin,end,ref(*this));
  }
  bool was_queued(VD v) {
    return get(loc,v) != NULL;
  }
  void relax(VD v,HD h,Cost &m) {
    if (c < m) {
      m=c;
      heapAdjustOrAdd(heap,v);
      put(pi,v,h);
    }
  }
  void reach(VD v) {
    Cost cv=get(mu,v);
    const Adj &a=adj[top.key];
    FOREACH(const ArcDest &ad,a) { // for each hyperarc v participates in as a tail
      HD h=a.harc;
      VD head=source(h,rev.g);
      RemainInf &ri=remain_infinum[h];      
        Assert(ri.cost() >= 0);
      ri.cost() += (a.multiplicity*cv);  // assess the cost of reaching v
        Assert(ri.remain() > 0);
      if (--ri.remain() == 0) // if v completes the hyperarc, attempt to use it to reach head (more cheaply)
        relax(head,h,ri.cost());            
    }

  }
  void finish() {
    typename Key::SetLocWeight save(ref(loc),mu);
    heapBuild(heap);
    while(heap.size()) {
      Key top=heapTop(heap);
      heapPop(heap);
      reach(top.key);
    }
  }  
 };


  // reachmap must be initialized to false by user
 template <
   class VertexReachMap=typename property_factory<G,VD>::rebind<bool>::reference
 >
 struct HyperGraphReach {
  Self &rev;
  VertexReachMap vr;
  HyperarcLeftMap tr;
  unsigned n;
  HyperGraphReach(Self &r,VertexReachMap v) : rev(r),vr(v),tr(r.unique_tails),n(0) {
    //copy_hyperarc_pmap(rev.g,rev.tails_remain_pmap(),tr);
    //relying on implementation same type (copy ctor)
  }
  void operator()(VD v) {
    if (get(vr,v))
      return;
    ++n;
    put(vr,v,true); // mark reached
    Adj &a=rev[v];
    for(Adj::iterator i=a.begin(),end=a.end();i!=end;++i) {
      HD harc=i->harc;
      VD tail=target(harc,g);
      if (!get(vr,tail)) { // not reached yet
        if (--tr[harc]==0)
          (*this)(source(harc,g)); // reach head
      }
    }

  }
  RemainPMap tails_remain_pmap() {
    return RemainPMap(tr);
  }

 };

 template <class P>
   unsigned reach(VD start,P p) {
     HyperGraphReach<P> alg(*this,p);
     alg(start);    
     return alg.n;
   }
  template <class B,class E,class VertexReachMap>
   unsigned reach(B begin,E end,VertexReachMap p) {
     HyperGraphReach<VertexReachMap> alg(*this,p);
     for_each(begin,end,ref(alg));    
     return alg.n;
   }
  

/* usage: 
BestTree alg(g,mu,pi);
alg.init_costs();
for each final (tail) vertex v with final cost f:
  alg(v,f);
alg.finish();

or ... assign to mu final costs yourself and then
alg.prepare(); // adds non-infinity cost only
alg.finish();

also
typename RemainInfCostMap::reference hyperarc_remain_and_cost_map() 
returns pmap with pmap[hyperarc].remain() == 0 if the arc was usable from the final tail set
and pmap[hyperarc].cost() being the cheapest cost to reaching all final tails
*/




};


#ifdef TEST
#include "test.hpp"
#endif

#ifdef TEST
BOOST_AUTO_UNIT_TEST( hypergraph )
{
}
#endif

#endif
