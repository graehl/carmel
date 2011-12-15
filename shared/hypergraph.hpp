#ifndef GRAEHL_SHARED__HYPERGRAPH_HPP
#define GRAEHL_SHARED__HYPERGRAPH_HPP
// like Boost Graph Library, but for hypergraphs (one head=target, multiple tails=sources).

/*
  ordered multi-hypergraph

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


//#include <graehl/tt/ttconfig.hpp>
//#include <graehl/tt/transducer.hpp>
//#include <graehl/shared/list.h>
//#include <graehl/shared/dynarray.h>
//#include <graehl/shared/weight.h>
#include <boost/ref.hpp>
#include <boost/graph/graph_traits.hpp>
#include <graehl/shared/property_factory.hpp>
#include <boost/iterator/counting_iterator.hpp>
//#include <graehl/shared/adjustableheap.hpp>
//#include <graehl/shared/graph.hpp>

namespace graehl {

template <class G>
struct path_traits {
  typedef float cost_type;
  static const bool viterbi = true; // means updates() sometimes returns false. a<b with combine(a,a)=a would suffice
  static inline cost_type unreachable() { return std::numeric_limits<cost_type>::infinity(); }
  static inline cost_type start() { return 0; }
  static inline cost_type extend(cost_type a,cost_type b) { return a+b; }
  static inline cost_type combine(cost_type a,cost_type b) { return std::min(a,b); }
  static inline bool update(cost_type const& a,cost_type &b) {
    if (a<b) {
      b=a; return true;
    }
    return false;
  }
  static inline cost_type repeat(cost_type a,float n) { return a*n; }
  static inline bool updates(cost_type a,cost_type b) { return a<b; } // note: this may be always true if you want to sum all paths until convergence e.g. combine=logplus(a,b) for LogWeight
  static inline bool includes(cost_type const& a,cost_type const& b) { return !updates(a,b); } // just used for debugging so far

};

// for graphs which have sources and not source - ordered multihypergraphs
template <class G>
struct edge_traits {
  typedef typename path_traits<G>::cost_type cost_type;
  typedef boost::graph_traits<G> GT;
  typedef unsigned tail_descriptor;
  typedef boost::counting_iterator<tail_descriptor> tail_iterator;
  typedef unsigned tails_size_type; // must always be unsigned. for now
};

/*
  free fns (ADL):


 */

template <class T>
struct hypergraph_traits : boost::graph_traits<T>,edge_traits<T> {};

template <class G>
struct updates_cost {
  typedef path_traits<G> PT;
  typedef typename PT::cost_type cost_type;
  typedef bool result_type;
  inline bool operator()(cost_type const& a,cost_type const& b) const {
    return PT::updates(a,b);
  }
};


#if 0
template <class T>
struct hypergraph_traits {
    typedef T graph;
    typedef boost::graph_traits<graph> GT;
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

template <class G>
inline
typename graph_object<G,hyperarc_tag_t>::iterator_pair begin_end(hyperarc_tag_t t,G &g) {
  return hyperarcs(g);
}
#endif

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
  typedef typename boost::graph_traits<G>::hyperarc_index_map HaIndex;
  typedef ArrayPMap<unsigned,HaIndex> PMap;
  typename PMap::Imp arc_remain(num_hyperarcs(g),HaIndex(g));
  TailsUpHypergraph<G,PMap> r(g,PMap(arc_remain));
*/

/*
  template <class G,class P1,class P2>
void copy_hyperarc_pmap(G &g,P1 a,P2 b) {
  visit(hyperarc_tag,make_indexed_copier(a,b));
}
*/

}//ns

#endif
