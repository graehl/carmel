#ifndef _HYPERGRAPH_HPP
#define _HYPERGRAPH_HPP

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
#include "property.hpp"
#include "transducer.hpp"
#include "list.h"
#include "dynarray.h"
#include "weight.h"
#include "byref.hpp"
#include "threadlocal.hpp"
#include "2heap.h"

struct VectorS {
  template <class T> struct container {
	typedef DynamicArray<T> type;
  }
};

struct ListS {
  template <class T> struct container {
	typedef List<T> type;
  }
};


template <class G,E,C,V>
struct SourceEdges;

template <class G,E,C,V>
struct graph_traits<SourceEdges<G,E,C,V> > : public graph_traits<G> {
  typedef E edge_descriptor;  
};


// for simplicity, requires vertex index ... could allow user to specify property map type instead, but would have to allocate it themself

template <class E,class G,class F>
void visit(G &g,F &f);


template <class G,class F>
void visit<typename graph_traits<G>::edge_descriptor>(G &g,F &f) {
  typedef typename graph_traits<G>::edge_iterator ei;
  std::pair<ei,ei> eis=edges(g);
  for (ei i=eis.first;i!=eis.second;++i)
  	f(*i);
}

template <class G,class F>
void visit<typename graph_traits<G>::vertex_descriptor>(G &g,F &f) {
  typedef typename graph_traits<G>::vertex_iterator ei;
  std::pair<ei,ei> eis=vertices(g);
  for (ei i=eis.first;i!=eis.second;++i)
  	f(*i);
}


// must define visit_edges (although default above should be ok)
template <class G,class E=typename graph_traits<G>::edge_descriptor,class ContS=VectorS,class VertexIndexer=typename graph_traits<G>::vertex_offset_map >
struct SourceEdges {    
  typedef SourceEdges<G,E,ContS,VertexIndexer> Self;
  typedef ContS::container<E>::type Adj;
  typedef FixedArray<Adj> Adjs;  
  typedef typename graph_traits<G>::vertex_descriptor vertex_descriptor;
  typedef typename E edge_descriptor;
  Adjs adj;
  typedef G graph;
  graph &g;
  operator graph &() { return g; }
  VertexIndexer vi;
  SourceEdges(Graph &g_,VertexIndexer vert_index=VertexIndexer()) : adj(g_.num_vertices()),g(g_), vi(vert_index) {	
	visit_edges<E>(g,*this);
  }
  unsigned index(vertex_descriptor v) {
    return get(vi,v);
  }
  Adj &operator[](vertex_descriptor v) {
    return adj[index(v)];
  }
  const Adj &operator[](vertex_descriptor v) const {
    return *(const_cast<Self *>(this))[v];
  }
/*  template <class I>
  populate(I beg,I end) {
	for (I i=beg;i!=end;++i)
	  (*this)(*i);
  }*/
  void operator()(E e) {
	unsigned i=get(vi,source(e,g));
	adj[i].push(e);
  }
};

#include <boost/counting_iterator.hpp>
template <class G,E,C,V>
struct graph_traits<SourceEdges<G,E,C,V> > : public graph_traits<G> {
{
  typedef SourceEdges<G,E,C,V> graph;
  //typedef typename graph::vertex_descriptor vertex_descriptor;
  //typedef typename graph::edge_descriptor edge_descriptor;
  typedef boost::counting_iterator_generator<graph::edge_desciptor> out_edge_iterator;
    typedef std::pair<
    typename graph_traits<Treexdcr<R> >::out_edge_iterator,
    typename graph_traits<Treexdcr<R> >::out_edge_iterator> pair_out_edge_it;    
};

template <class G,E,C,V>
inline typename SourceEdges<G,E,C,V>::pair_out_edge_it
out_edges(	  
          typename graph_traits<G>::vertex_descriptor v,
	      const SourceEdges<G,E,C,V> &g
	   )
{
  typedef typename SourceEdges<G,E,C,V> Self;
  typename Self::Adj &adj=g[v];
  return typename Self::pair_out_edge_it(adj.begin(),adj.end());
}

template <class G,E,C,V>
unsigned out_degree(typename SourceEdges<G,E,C,V>::vertex_descriptor v,SourceEdges<G,E,C,V> &g) {
  return g[v].size();
}

template <class G,E,C,V,F>
inline void
visit<E>(	  
          typename graph_traits<G>::vertex_descriptor v,
	      const SourceEdges<G,E,C,V> &g, 
          F f
	   )
{
  typedef typename SourceEdges<G,E,C,V> Self;
  typename Self::Adj &adj=g[v];
  for (typename Self::out_edge_iterator i=adj.begin(),e=adj.end();i!=e;++i)
    f(*i);  
}


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



//! NOTE: default comparison direction is reversed ... making max-heaps into min-heaps (as desired for best-tree) and vice versa
// loc[key] = void * (or DistToState<K,W,L> *)
// weight[key] = some weight class
template <class K,class W,class L>
struct HeapKey {
  typedef K value_type;
  typedef W weightmap_type;
  typedef L locmap_type;
  typedef DistToState<K,W,L> Self;
  typedef Self *Loc;
  typedef typename W::value_type weight_type;
  K key;
  static THREADLOCAL L loc;
  static THREADLOCAL W weight;
  struct SetLocWeight {
    L old_loc;
    W old_weight;
    SetLocWeight(L l,W w) : old_loc(loc),old_weight(weight) {
      loc=l;
      weight=w;
    }
    ~SetLocWeight() {
      loc=old_loc;
      weight=old_weight;
    }
  };
  HeapKey() : key() {}
  HeapKey(K k) : key(k) {}
  HeapKey(Self s) : key(s.k) {}

  void *loc() const {
    return loc[key];
  }
  weight_type weight() const {
    return weight[key];
  }
//  static FLOAT_TYPE unreachable;
  //operator weight_type() const { return weight[key]; }
  //operator K () const { return key; }
  void operator = (HeapKey<K,W,L> rhs) { 
    loc[rhs.key] = this;
    loc = rhs.loc;
  }
};

#ifdef MAIN
template<class K,W,L>
THREADLOCAL typename HeapKey<K,W,L>::weightmap_type HeapKey<K,W,L>::weight;

template<class K,W,L>
THREADLOCAL typename HeapKey<K,W,L>::locmap_type HeapKey<K,W,L>::loc;
#endif

template<class K,W,L>
inline bool operator < (HeapKey<K,W,L> lhs, HeapKey<K,W,L> rhs) {
  return lhs.weight() > rhs.weight();
}

/*
template<class K,W,L>
 inline bool operator == (HeapKey<K,W,L> lhs, HeapKey<K,W,L> rhs) {
  return HeapKey<K,W,L>::weight[lhs.key] == HeapKey<K,W,L>::weight[rhs.key];
}

template<class K,W,L>
inline bool operator == (HeapKey<K,W,L> lhs, K rhs) {
  return HeapKey<K,W,L>::weight[lhs.key] == rhs;
}
*/

/*
template <class V,class O>
struct ArrayPMap {
  typedef boost::reference_type<ArrayPMapImp<V,O> > type;
};
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
  typedef typename graph_traits<G>::hyperarc_offset_map HaIndex;
  typedef ArrayPMapImp<unsigned,HaIndex> PMapImp;
  typedef typename PMapImp::property_map PMap;
  PMapImp arc_remain(num_hyperarcs(g),HaIndex(g));
  ReverseHypergraph<G,PMap> r(g,arc_remain);
*/

template <class G,class P1,class P2>
void copy_hyperarc_pmap(G &g,P1 a,P2 b) {
  visit<typename graph_traits<G>::hyperarc_descriptor>(IndexedCopier(a,b));
}



template <class G,class HyperarcLeftMap=ArrayPMap<unsigned,typename graph_traits<G>::hyperarc_offset_map>,class VertexIndexer=typename graph_traits<G>::vertex_offset_map,class ContS=VectorS >
struct ReverseHyperGraph {
  typedef ReverseHyperGraph<G,HyperarcLeftMap,VertexIndexer,ContS> Self;
  typedef G graph;
  typedef typename graph_traits<graph>::hyperarc_descriptor hd;    

  struct ArcDest  {
	hd harc; // hyperarc with this tail
	unsigned multiplicity; // tail multiplicity
	ArcDest(hd e) : harc(e), multiplicity(1) {}
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
  
  typedef ContS::container<ArcDest>::type Adj;
  typedef FixedArray<Adj> Adjs;
  Adjs adj;  
  VertexIndexer vi;
  unsigned vertindex(vertex_descriptor v) const {
    return get(vi,v);
  }
  HyperarcLeftMap unique_tails;
  
    
  graph &g;
  ReverseHyperGraph(const G& g_,HyperarcLeftMap unique_tails_pmap,VertexIndexer vert_index=VertexIndexer()) : adj(num_vertices(g)), g(g_), vi(vert_index), unique_tails(unique_tails_pmap)
    //num_hyperarcs(g_)
  {		
	visit_edges<hd>(g,*this);
  }
  typedef typename graph_traits::vertex_descriptor vertex_descriptor;
  Adj &operator[](vertex_descriptor v) {
    return adj[get(vi,index(v)];
  }
  void operator()(hd harc) {
    unsigned ut=0;
    for (typename graph_traits<graph>::pair_tail_iterator pti=tails(harc,g);
      pti.first!=pti.second; ++pti.first) {
        //Adj &a=adj[get(vi,target(*(pti.first),g))]; // adjacency list for tail
        Adj &a=(*this)[target(*(pti.first),g)];
        if (a.size()&&a.top().harc == ha) {
          // last hyperarc with same tail = same hyperarc
	      ++a.top().multiplicity;
        } else {	  // new (unique) tail
	      a.push(harc); // default multiplicity=1
	      ++ut; 
	    }
      }
    put(unique_tails,harc,ut);
  }  
  /*
  template <class E>
  struct CountTails {
    E tails_rem;
    CountTails(E tails_rem_) : tails_rem(tails_rem_) {}
    operator()(hd harc) {
          unsigned ut=0;
    for (typename graph_traits<graph>::pair_tail_iterator pti=tails(harc,g); pti.first!=pti.second; ++pti.first) {
    }

    }
  };
  template <class E>
  void count_unique_tails(E e) {
    visit_edges<hd>(g,CountTails<E>(e));
  }
  */
  // user must init to 0
  template <class E>
  void count_unique_tails(E e) {
    FOREACH(const Adj &a,adj) {
      FOREACH(const ArcDest &ad,a) {
        ++e[ad.harc];
      }
    }    
  }
  //FIXME:destroys unique_tails instead of making copy.
  // reachmap must be initialized to false by user
 template <class VertexReachMap,class TailsRemainMap>
 struct HyperGraphReach {
  Self &rev;
  VertexReachMap vr;
  TailsRemainMap tr;
  unsigned n;
  HyperGraphReach(Self &r,VertexReachMap v,TailsRemainMap t) : rev(r),vr(v),tr(t),n(0) {
    copy_hyperarc_pmap(rev.g,rev.unique_tails,tr);
  }
  void operator()(vertex_descriptor v) {
    if (get(vr,v))
      return;
    ++n;
    put(vr,v,true); // mark reached
    Adj &a=rev[v];
    for(Adj::iterator i=a.begin(),end=a.end();i!=end;++i) {
      hd harc=i->harc;
      vertex_descriptor tail=target(harc,g);
      if (!get(vr,tail)) { // not reached yet
        if (--tr[harc]==0)
          (*this)(source(harc,g)); // reach head
      }
    }
  }

 };
 template <class P>
   unsigned reach(vertex_descriptor start,P p) {
     HyperGraphReach<P> alg(*this,p);
     alg(start);    
     return alg.n;
   }
  template <class B,class E,class VertexReachMap>
   unsigned reach(B begin,E end,VertexReachMap p) {
     HyperGraphReach<VertexReachMap> alg(*this,p);
     for_each(begin,end,boost::ref(alg));    
     return alg.n;
   }
  

//FIXME:destroys unique_tails instead of making copy.
 // costmap must be initialized to initial costs (for start vertices) or infinity (otherwise) by user
 // edgecostmap should be initialized to edge costs
 template <class VertexCostMap,class PredMap>
 struct BestTree {
  Self &rev;
  VertexCostMap mu;
  TailsRemainMap tr;
  
  PredMap pi;
  LocMap loc;
  typedef HeapKey<vertex_descriptor,VertexCostMap,LocMap> Key;
  FixedArray<Key *> Locs;
  Locs loc;

  typedef DynamicArray<Key> Heap;
  Heap heap;
  
  BestTree(Self &rev_,VertexCostMap mu_,EdgeCostMap PredMap pi_,LocMap lc_,TailsRemainMap t) :
    rev(rev_),mu(mu_),pi(pi_),tr(t),loc(lc_),heap(num_vertices(rev.g)) {
      copy_hyperarc_pmap(rev.g,rev.unique_tails,tr);      
    }   
  void operator()(vertex_descriptor v) {
    heap.push_back(v);
  }
  void prepare() {
    visit<typename graph_traits<G>::vertex_descriptor>(rev.g,boost::ref(*this));
  }
  template<class I>
  void prepare(I begin, I end) {
    std::foreach(begin,end,boost::ref(*this));
  }
  void finish() {
    typename Key::SetLocWeight save(loc,mu);
    typedef typename Key::SetLocWeight::weight_type Cost;
    heapBuild(heap.begin(),heap.end());
    while(heap.size()) {
      Key top=heapTop(heap.begin());
      heapPop(heap.begin(),heap.end());
      heap.pop_back();
      Adj &a=adj[rev.vertindex(top.key)];
      FOREACH(const ArcDest &ad,a) {        
        Cost &c=mu[a.harc];
        vertex_descriptor head=source(a.harc,rev.g);
        if (c < 

      }
    }
  }
  
 };

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
