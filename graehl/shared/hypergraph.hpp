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

    like Boost Graph Library, but for hypergraphs (one head=target, multiple tails=sources). useful especially
  for lazy_forest_kbest.hpp

  ordered multi-hypergraph

  G A type that is a model of Graph.
  g An object of type G.
  v An object of type boost::graph_traits<G>::vertex_descriptor.

  Associated Types
  boost::graph_traits<G>::traversal_category

  This tag type must be convertible to adjacency_graph_tag.

  boost::graph_traits<G>::adjacency_iterator

  An adjacency iterator for a vertex v provides access to the vertices adjacent to v. As such, the value type
  of an adjacency iterator is the vertex descriptor type of its graph. An adjacency iterator must meet the
  requirements of MultiPassInputIterator.
  Valid Expressions
  adjacent_vertices(v, g)         Returns an iterator-range providing access to the vertices adjacent to
  vertex v in graph g.[1]
  Return type: std::pair<adjacency_iterator, adjacency_iterator>


  http://www.boost.org/libs/graph/doc/adjacency_list.html

*/

#ifndef GRAEHL_SHARED__HYPERGRAPH_HPP
#define GRAEHL_SHARED__HYPERGRAPH_HPP
#pragma once

#include <graehl/shared/graph.hpp>  // graph_object

#include <boost/graph/graph_traits.hpp>
#include <boost/range/iterator_range.hpp>
#include <graehl/shared/path_traits.hpp>

namespace graehl {

struct hyperarc_tag {
  hyperarc_tag() {}
};

namespace {
hyperarc_tag const hyperarcT;
}


template <class T>
struct hypergraph_traits : boost::graph_traits<T>, edge_traits<T> {
  typedef T graph;
  typedef boost::graph_traits<graph> GT;
  typedef typename graph::hyperarc_descriptor hyperarc_descriptor;
  typedef typename graph::hyperarc_iterator hyperarc_iterator;
  typedef typename graph::tail_descriptor tail_descriptor;
  typedef typename graph::tail_iterator tail_iterator;
  typedef typename GT::vertex_iterator vertex_iterator;
  typedef typename GT::edge_iterator edge_iterator;

  typedef boost::iterator_range<tail_iterator> pair_tail_it;
  typedef boost::iterator_range<hyperarc_iterator> pair_hyperarc_it;
  typedef boost::iterator_range<vertex_iterator> pair_vertex_it;
  typedef boost::iterator_range<edge_iterator> pair_edge_it;
};

template <class G>
struct graph_object<G, hyperarc_tag> {
  typedef typename hypergraph_traits<G>::hyperarc_descriptor descriptor;
  typedef typename hypergraph_traits<G>::hyperarc_iterator iterator;
  typedef boost::iterator_range<iterator> iterator_pair;
};

template <class G>
inline typename graph_object<G, hyperarc_tag>::iterator_pair begin_end(hyperarc_tag, G& g) {
  return hyperarcs(g);
}

/*
  struct NoWeight {
  void setOne() {}
  };
*/

// use boost::iterator_property_map<RandomAccessIterator, OffsetFeatures, T, R> with OffsetFeatures doing the
// index mapping
// usually: K = key *, you have array of key at key *: vec ... vec+size
// construct OffsetArrayPmap(vec, vec+size) and get an array of size Vs (default constructed)

/*Iterator Must be a model of Random Access Iterator.
  OffsetFeatures Must be a model of Readable Property Map and the value type must be convertible to the
  difference type of the iterator.
  T The value type of the iterator.         std::iterator_traits<RandomAccessIterator>::value_type
  R The reference type of the iterator.     std::iterator_traits<RandomAccessIterator>::reference

  iterator_property_map(Iterator i, OffsetFeatures m)*/


/*template <class V, class O>
  struct ArrayPMap;
*/


/*: public ByRef<ArrayPMapImp<V, O> >
  {
  typedef ArrayPMapImp<V, O> Imp;
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
  typedef ArrayPMap<unsigned, HaIndex> PMap;
  typename PMap::Imp arc_remain(num_hyperarcs(g), HaIndex(g));
  TailsUpHypergraph<G, PMap> r(g, PMap(arc_remain));
*/

/*
  template <class G, class P1, class P2>
  void copy_hyperarc_pmap(G &g, P1 a, P2 b) {
  visit(hyperarc_tag, make_indexed_copier(a, b));
  }
*/


}

#endif
