#ifndef GRAPH_HPP
#define GRAPH_HPP

// graph_object<G,(vertex|edge|hyperarc)_t>, visit(edge_tag,g) ...


#include <boost/graph/graph_traits.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include "byref.hpp"

#include "property.hpp"
#include "container.hpp"





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

template <class G> struct graph_object<G,vertex_tag_t> {
    typedef typename graph_traits<G>::vertex_descriptor descriptor;
    typedef typename graph_traits<G>::vertex_iterator iterator;
    typedef std::pair<iterator,iterator> iterator_pair;
   };

template <class Tag,class G,class E,class F>
void visit(Tag t,G &g,F f);

template <class Tag,class G,class E>
inline
typename graph_object<G,Tag>::iterator_pair begin_end(Tag t,G &g) {
  return typename graph_object<G,Tag>::iterator_pair
    (begin(t,g),end(t,g));
}

template <class Tag,class G,class E>
inline
typename graph_object<G,Tag>::iterator_pair begin_end(vertex_tag_t t,G &g) {
  return vertices(g);
}

template <class Tag,class G,class E>
inline
typename graph_object<G,Tag>::iterator_pair begin_end(edge_tag_t t,G &g) {
  return edges(g);
}


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



#endif
