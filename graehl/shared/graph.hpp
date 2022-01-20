// Copyright 2014 Jonathan Graehl-http://graehl.org/
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

    visit and begin_end for Boost Graph Library graphs (for consistency with hypergraph.hpp).

    graph_object<G, (vertex|edge|hyperarc)_t>, visit(edge_tag, g) ...
*/

#ifndef GRAEHL_SHARED__GRAPH_HPP
#define GRAEHL_SHARED__GRAPH_HPP
#pragma once


#include <boost/graph/graph_traits.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/iterator_range.hpp>

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
} // namespace
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

template <class Tag, class G, class F>
void visit(Tag t, G& g, F& f);

#if 0
template <class Tag, class G>
inline typename graph_object<G, Tag>::iterator_pair begin_end(Tag t, G& g) {
  return typename graph_object<G, Tag>::iterator_pair(boost::begin(t, g), boost::end(t, g));
}
#endif
template <class G>
inline typename graph_object<G, vertex_tag>::iterator_pair begin_end(vertex_tag, G& g) {
  return vertices(g);
}

template <class G>
inline typename graph_object<G, edge_tag>::iterator_pair begin_end(edge_tag, G& g) {
  return edges(g);
}

// const g
template <class G>
inline typename graph_object<G, vertex_tag>::iterator_pair begin_end(vertex_tag, G const& g) {
  return vertices(g);
}

template <class G>
inline typename graph_object<G, edge_tag>::iterator_pair begin_end(edge_tag, G const& g) {
  return edges(g);
}

// TEST: does V v mean we get a chance to V &v or V const& v ???

template <class T, class G, class F>
inline void visit(T t, G& g, F& f) {
  auto const& pi = begin_end(t, g);
  for (auto i = boost::begin(pi), e = boost::end(pi); i != e; ++i)
    f(*i);
}

template <class T, class G, class F>
inline void visit(T t, G& g, F const& f) {
  auto const& pi = begin_end(t, g);
  for (auto i = boost::begin(pi), e = boost::end(pi); i != e; ++i)
    f(*i);
}

template <class T, class G, class F>
inline void visit(T t, G const& g, F& f) {
  auto const& pi = begin_end(t, g);
  for (auto i = boost::begin(pi), e = boost::end(pi); i != e; ++i)
    f(*i);
}

template <class T, class G, class F>
inline void visit(T t, G const& g, F const& f) {
  auto const& pi = begin_end(t, g);
  for (auto i = boost::begin(pi), e = boost::end(pi); i != e; ++i)
    f(*i);
}


} // namespace graehl

#endif
