/** \file

    visit and begin_end for Boost Graph Library graphs (for consistency with hypergraph.hpp).

    graph_object<G, (vertex|edge|hyperarc)_t>, visit(edge_tag, g) ...
*/

#ifndef GRAEHL_SHARED__GRAPH_HPP
#define GRAEHL_SHARED__GRAPH_HPP
#pragma once


#include <boost/graph/graph_traits.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

namespace graehl {

struct edge_tag {
  edge_tag() {}
};

struct vertex_tag {
  vertex_tag() {}
};

#include <graehl/shared/warning_compiler.h>
CLANG_DIAG_OFF(unused-variable)
namespace {
edge_tag const edgeT;
vertex_tag const vertexT;
}
CLANG_DIAG_ON(unused-variable)

template <class G, class T>
struct graph_object;

template <class G>
struct graph_object<G, edge_tag> {
  typedef typename boost::graph_traits<G>::edge_descriptor descriptor;
  typedef typename boost::graph_traits<G>::edge_iterator iterator;
  typedef boost::iterator_range<iterator> iterator_pair;
};

template <class G>
struct graph_object<G, vertex_tag> {
  typedef typename boost::graph_traits<G>::vertex_descriptor descriptor;
  typedef typename boost::graph_traits<G>::vertex_iterator iterator;
  typedef boost::iterator_range<iterator> iterator_pair;
};

template <class Tag, class G, class E, class F>
void visit(Tag t, G& g, F f);

template <class Tag, class G, class E>
inline typename graph_object<G, Tag>::iterator_pair begin_end(Tag t, G& g) {
  return typename graph_object<G, Tag>::iterator_pair(boost::begin(t, g), boost::end(t, g));
}

template <class Tag, class G, class E>
inline typename graph_object<G, Tag>::iterator_pair begin_end(vertex_tag, G& g) {
  return vertices(g);
}

template <class Tag, class G, class E>
inline typename graph_object<G, Tag>::iterator_pair begin_end(edge_tag, G& g) {
  return edges(g);
}

// const g
template <class Tag, class G, class E>
inline typename graph_object<G, Tag>::iterator_pair begin_end(vertex_tag, G const& g) {
  return vertices(g);
}

template <class Tag, class G, class E>
inline typename graph_object<G, Tag>::iterator_pair begin_end(edge_tag, G const& g) {
  return edges(g);
}


// TEST: does V v mean we get a chance to V &v or V const& v ???

template <class G, class T, class F>
inline void visit(T, G& g, F const& f) {
  using namespace boost;
  typename graph_object<G, T>::iterator_pair pi = vertices(g);
  for (typename graph_object<G, T>::iterator i = boost::begin(pi), e = boost::end(pi); i != e; ++i) f(*i);
}

template <class G, class T, class F>
inline void visit(T, G& g, F& f) {
  using namespace boost;
  typename graph_object<G, T>::iterator_pair pi = vertices(g);
  for (typename graph_object<G, T>::iterator i = boost::begin(pi), e = boost::end(pi); i != e; ++i) f(*i);
}

// const g
template <class G, class T, class F>
inline void visit(T, G const& g, F const& f) {
  using namespace boost;
  typename graph_object<G, T>::iterator_pair pi = vertices(g);
  for (typename graph_object<G, T>::iterator i = boost::begin(pi), e = boost::end(pi); i != e; ++i) f(*i);
}

template <class G, class T, class F>
inline void visit(T, G const& g, F& f) {
  using namespace boost;
  typename graph_object<G, T>::iterator_pair pi = vertices(g);
  for (typename graph_object<G, T>::iterator i = boost::begin(pi), e = boost::end(pi); i != e; ++i) f(*i);
}


}

#endif
