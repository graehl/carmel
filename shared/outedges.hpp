// (regular) graph of the edges in a (hyper)graph with the same head.
#ifndef OUTEDGES_HPP
#define OUTEDGES_HPP

#include <boost/graph/graph_traits.hpp>
#include <graehl/shared/hypergraph.hpp>

//OutEdges (generic out_edge_iterator graph adapter, out_edges ...) and

namespace graehl {
/*
template <class G,class E,class C,class V>
struct OutEdges;
*/
// for simplicity, requires vertex index ... could allow user to specify property map type instead, but would have to allocate it themself

// see property.hpp for factory
// must define visit (although default above should be ok)
template <class G,class T=edge_tag,class ContS=VectorS,
          class VertMapFactory=property_factory<G,vertex_tag>
>
struct OutEdges {
  typedef OutEdges<G,T,ContS,VertMapFactory> Self;
    typedef typename boost::graph_traits<G>::vertex_descriptor vertex_descriptor;
  typedef typename graph_object<G,T>::descriptor edge_descriptor;
  typedef typename ContS::template container<edge_descriptor>::type Adj;

  typedef G graph_type;
  graph_type &g;
  graph_type &graph() { return g; }
  operator graph_type &() { return g; }
  typedef typename VertMapFactory::template rebind<Adj>::implementation Adjs;
  Adjs adj;
  OutEdges(graph_type &g_) :
  g(g_),adj(VertMapFactory(g_))
  {
        visit(T(),g,ref(*this));
  }
  OutEdges(graph_type &g_,VertMapFactory vert_fact) :
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
}//ns

namespace boost {
template <class G,class E,class C,class V>
struct graph_traits<graehl::OutEdges<G,E,C,V> > : public boost::graph_traits<G> {
  typedef G parent_graph;
    typedef boost::graph_traits<parent_graph> GT;
    typedef graehl::OutEdges<G,E,C,V> graph;
  typedef typename graph::edge_descriptor edge_descriptor;
  //typedef boost::counting_iterator<edge_descriptor *> out_edge_iterator;
  typedef typename graph::Adj::iterator out_edge_iterator;
  typedef std::pair<out_edge_iterator,out_edge_iterator> pair_out_edge_it;
};
}//ns


namespace graehl {

template <class G,class E,class C,class V>
struct hypergraph_traits<OutEdges<G,E,C,V> > : public hypergraph_traits<G> {
  //  typedef OutEdges<G,E,C,V> graph;
};

template <class G,class E,class C,class V>
inline typename boost::graph_traits<OutEdges<G,E,C,V> >::pair_out_edge_it
out_edges(
    typename boost::graph_traits<OutEdges<G,E,C,V> >::vertex_descriptor v,
              OutEdges<G,E,C,V> &g
           )
{
  typedef OutEdges<G,E,C,V> Self;
  typename Self::Adj &adj=g[v];
  return typename boost::graph_traits<Self>::pair_out_edge_it(adj.begin(),adj.end());
}

template <class G,class E,class C,class V>
unsigned out_degree(typename OutEdges<G,E,C,V>::vertex_descriptor v,OutEdges<G,E,C,V> &g) {
  return g[v].size();
}

template <class G,class E,class C,class V,class F>
inline void
visit_out(
    typename boost::graph_traits<G>::vertex_descriptor v,
              const OutEdges<G,E,C,V> &g,
          F f
           )
{
  typedef OutEdges<G,E,C,V> Self;
  typename Self::Adj &adj=g[v];
  for (typename Self::out_edge_iterator i=adj.begin(),e=adj.end();i!=e;++i)
    f(*i);
}

}

//! TESTS IN TRANSDUCERGRAPH.HPP

#endif
