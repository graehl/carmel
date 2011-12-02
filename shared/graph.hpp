// visit and begin_end for Boost Graph Library graphs (for consistency with hypergraph.hpp)
#ifndef GRAEHL_SHARED__GRAPH_HPP
#define GRAEHL_SHARED__GRAPH_HPP

// graph_object<G,(vertex|edge|hyperarc)_t>, visit(edge_tag,g) ...


#include <boost/graph/graph_traits.hpp>
//#include <boost/iterator/counting_iterator.hpp>
//#include <graehl/shared/byref.hpp>
//#include <graehl/shared/property_factory.hpp>


namespace graehl {

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

struct edge_tag {};

static edge_tag edgeT;

struct vertex_tag {};

static vertex_tag vertexT;

template <class G,class T> struct graph_object;


template <class G> struct graph_object<G,edge_tag> {
    typedef typename boost::graph_traits<G>::edge_descriptor descriptor;
    typedef typename boost::graph_traits<G>::edge_iterator iterator;
    typedef std::pair<iterator,iterator> iterator_pair;
};

template <class G> struct graph_object<G,vertex_tag> {
    typedef typename boost::graph_traits<G>::vertex_descriptor descriptor;
    typedef typename boost::graph_traits<G>::vertex_iterator iterator;
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
typename graph_object<G,Tag>::iterator_pair begin_end(vertex_tag ,G &g) {
  return vertices(g);
}

template <class Tag,class G,class E>
inline
typename graph_object<G,Tag>::iterator_pair begin_end(edge_tag ,G &g) {
  return edges(g);
}

// const g
template <class Tag,class G,class E>
inline
typename graph_object<G,Tag>::iterator_pair begin_end(vertex_tag ,G const&g) {
  return vertices(g);
}

template <class Tag,class G,class E>
inline
typename graph_object<G,Tag>::iterator_pair begin_end(edge_tag ,G const&g) {
  return edges(g);
}


//TEST: does V v mean we get a chance to V &v or V const& v ???

template <class G,class T,class F>
inline void visit(T ,G &g,F const& f) {
  for (typename graph_object<G,T>::iterator_pair pi=
         vertices(g);
       pi.first!=pi.second;
       ++pi.first)
    f(*(pi.first));
}

template <class G,class T,class F>
inline void visit(T ,G &g,F &f) {
  for (typename graph_object<G,T>::iterator_pair pi=
         vertices(g);
       pi.first!=pi.second;
       ++pi.first)
    f(*(pi.first));
}

// const g
template <class G,class T,class F>
inline void visit(T ,G const&g,F const& f) {
  for (typename graph_object<G,T>::iterator_pair pi=
         vertices(g);
       pi.first!=pi.second;
       ++pi.first)
    f(*(pi.first));
}

template <class G,class T,class F>
inline void visit(T ,G const&g,F &f) {
  for (typename graph_object<G,T>::iterator_pair pi=
         vertices(g);
       pi.first!=pi.second;
       ++pi.first)
    f(*(pi.first));
}

}//ns


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
   struct object<edge_tag> {
    typedef typename graph_traits<G>::edge_descriptor descriptor;
    typedef typename graph_traits<G>::edge_iterator iterator;
    typedef std::pair<iterator,iterator> iterator_pair;
   };
  template <>
   struct object<hyperarc_tag> {
    typedef typename graph_traits<G>::hyperarc_descriptor descriptor;
    typedef typename graph_traits<G>::hyperarc_iterator iterator;
    typedef std::pair<iterator,iterator> iterator_pair;
   };
  template <>
   struct object<vertex_tag> {
    typedef typename graph_traits<G>::vertex_descriptor descriptor;
    typedef typename graph_traits<G>::vertex_iterator iterator;
    typedef std::pair<iterator,iterator> iterator_pair;
   };
};
*/

#endif
