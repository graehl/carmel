#ifndef _GRAPH_HPP
#define _GRAPH_HPP

#include <boost/graph/graph_traits.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include "byref.hpp"

#include "property.hpp"
#include "dynarray.h"
#include "list.h"

struct VectorS {
  template <class T> struct container {
        typedef DynamicArray<T> type;
  };
};

struct ListS {
  template <class T> struct container {
        typedef List<T> type;
  };
};


template <class G,class E,class C,class V>
struct SourceEdges;

// for simplicity, requires vertex index ... could allow user to specify property map type instead, but would have to allocate it themself

/*
if graphs had iterator_range
graph traits had (edge|vertex)::descriptor, (edge|vertex)::iterator
template <class E,class G,class F>
void visit(G &g,F f) {
  typedef typename E::iterator ei;
  std::pair<ei,ei> eis=iterator_range<E>(g);
  for (ei i=eis.first;i!=eis.second;++i)
        f(*i);
}
*/

struct edge_tag_t {
};

static edge_tag_t edge_tag;

struct hyperarc_tag_t {
};

static hyperarc_tag_t hyperarc_tag;

struct vertex_tag_t {
};

static vertex_tag_t vertex_tag;

/*
What you want to do is just not possible: you can only specialize the
inner template for a fully specialized outer template. It doesn't work
the other way round. See 14.7.3/18.

template <typename E>
struct F {
    template<typename G> class H;
};

template <>
template<typename E>
class F<int>::H<E> {};
*/

/*
template <class G>
struct graph_types {
  template <class T> struct object;
  template <>
   struct object<edge_tag_t> {
    typedef typename graph_traits<G>::edge_descriptor descriptor;
    typedef typename graph_traits<G>::edge_iterator iterator;
    typedef std::pair<iterator,iterator> iterator_pair;
   };
  template <>
   struct object<hyperarc_tag_t> {
    typedef typename graph_traits<G>::hyperarc_descriptor descriptor;
    typedef typename graph_traits<G>::hyperarc_iterator iterator;
    typedef std::pair<iterator,iterator> iterator_pair;
   };
  template <>
   struct object<vertex_tag_t> {
    typedef typename graph_traits<G>::vertex_descriptor descriptor;
    typedef typename graph_traits<G>::vertex_iterator iterator;
    typedef std::pair<iterator,iterator> iterator_pair;
   };
};
*/

template <class G,class T> struct graph_object;

template <class G> struct graph_object<G,edge_tag_t> {
    typedef typename graph_traits<G>::edge_descriptor descriptor;
    typedef typename graph_traits<G>::edge_iterator iterator;
    typedef std::pair<iterator,iterator> iterator_pair;
};

template <class G> struct graph_object<G,hyperarc_tag_t> {
    typedef typename graph_traits<G>::hyperarc_descriptor descriptor;
    typedef typename graph_traits<G>::hyperarc_iterator iterator;
    typedef std::pair<iterator,iterator> iterator_pair;
   };

template <class G> struct graph_object<G,vertex_tag_t> {
    typedef typename graph_traits<G>::vertex_descriptor descriptor;
    typedef typename graph_traits<G>::vertex_iterator iterator;
    typedef std::pair<iterator,iterator> iterator_pair;
   };

template <class Tag,class G,class E,class F>
void visit(Tag t,G &g,F f);

/*
template <class G,class F>
inline void visit(edge_tag_t unused,G &g,F f) {
  typedef typename graph_traits<G>::edge_iterator ei;
  std::pair<ei,ei> eis=edges(g);
  for (ei i=eis.first;i!=eis.second;++i)
        f(*i);
}

template <class G,class F>
inline void visit(vertex_tag_t unused,G &g,F f) {
  typedef typename graph_traits<G>::vertex_iterator ei;
  std::pair<ei,ei> eis=vertices(g);
  for (ei i=eis.first;i!=eis.second;++i)
        f(*i);
}
*/

template <class G,class T,class F>
inline void visit(T unused_type_tag,G &g,F f) {
  for (typename graph_object<G,T>::iterator_pair pi=
    vertices(g);
   pi.first!=pi.second;
   ++pi.first)
        deref(f)(*(pi.first));
}


// see property.hpp for factory
// must define visit (although default above should be ok)
template <class G,class T=edge_tag_t,class ContS=VectorS,
class VertMapFactory=property_factory<G,vertex_tag_t>
>
struct SourceEdges {
  typedef SourceEdges<G,T,ContS,VertMapFactory> Self;
  typedef typename graph_traits<G>::vertex_descriptor vertex_descriptor;
  typedef typename graph_object<G,T>::descriptor edge_descriptor;
  typedef typename ContS::container<edge_descriptor>::type Adj;
  //typedef FixedArray<Adj> Adjs;


  typedef G graph_type;
  graph_type &g;
  graph_type &graph() { return g; }
  operator graph_type &() { return g; }
  typedef typename VertMapFactory::rebind<Adj>::implementation Adjs;
  Adjs adj;
  SourceEdges(graph_type &g_) :
  g(g_),adj(VertMapFactory(g_))
  {
        visit(T(),g,ref(*this));
  }
  SourceEdges(graph_type &g_,VertMapFactory vert_fact) :
  g(g_),adj(vert_fact)
  {
        visit(T(),g,ref(*this));
  }
  Adj &operator[](vertex_descriptor v) {
    return adj[v];
  }
  const Adj &operator[](vertex_descriptor v) const {
    return *(const_cast<Self *>(this))[v];
  }
/*  template <class I>
  populate(I beg,I end) {
        for (I i=beg;i!=end;++i)
          (*this)(*i);
  }*/
  void operator()(edge_descriptor e) {
        adj[source(e,g)].push(e);
  }
};

namespace boost {
template <class G,class E,class C,class V>
struct graph_traits<SourceEdges<G,E,C,V> > : public graph_traits<G> {
  typedef G parent_graph;
  typedef graph_traits<parent_graph> GT;
  typedef SourceEdges<G,E,C,V> graph;
  typedef typename graph::edge_descriptor edge_descriptor;
  //typedef boost::counting_iterator<edge_descriptor *> out_edge_iterator;
  typedef typename graph::Adj::iterator out_edge_iterator;
    typedef std::pair<out_edge_iterator,out_edge_iterator> pair_out_edge_it;
};
};



template <class G,class E,class C,class V>
inline typename graph_traits<SourceEdges<G,E,C,V> >::pair_out_edge_it
out_edges(
          typename graph_traits<SourceEdges<G,E,C,V> >::vertex_descriptor v,
              SourceEdges<G,E,C,V> &g
           )
{
  typedef SourceEdges<G,E,C,V> Self;
  typename Self::Adj &adj=g[v];
  return typename graph_traits<Self>::pair_out_edge_it(adj.begin(),adj.end());
}

template <class G,class E,class C,class V>
unsigned out_degree(typename SourceEdges<G,E,C,V>::vertex_descriptor v,SourceEdges<G,E,C,V> &g) {
  return g[v].size();
}

template <class G,class E,class C,class V,class F>
inline void
visit_out(
          typename graph_traits<G>::vertex_descriptor v,
              const SourceEdges<G,E,C,V> &g,
          F f
           )
{
  typedef SourceEdges<G,E,C,V> Self;
  typename Self::Adj &adj=g[v];
  for (typename Self::out_edge_iterator i=adj.begin(),e=adj.end();i!=e;++i)
    f(*i);
}


//! TESTS IN TRANSDUCERGRAPH.HPP

#endif
