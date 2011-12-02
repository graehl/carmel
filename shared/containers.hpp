#ifndef GRAEHL_SHARED__CONTAINERS_HPP
#define GRAEHL_SHARED__CONTAINERS_HPP

/* container selectors (all assume default allocator, but TODO - could change to template on allocator).

   usage:

   template <class V,class contS>
   typename contS::template container<V>::type c;

*/

#include <boost/config.hpp>

#include <list>
#ifdef BOOST_HAS_SLIST
#include BOOST_SLIST_HEADER
# define GRAEHL_STD_SLIST BOOST_STD_EXTENSION_NAMESPACE::slist
#else
# define GRAEHL_STD_SLIST std::list
#endif

#include <vector>
#include <map>
#include <set>

#include <boost/functional/hash.hpp>

#ifndef UNORDERED_NS
#ifndef USE_TR1_UNORDERED
#if defined(WIN32)
# define USE_TR1_UNORDERED 0
#else
# define USE_TR1_UNORDERED 1
// in gcc, std::tr1 is faster than boost unordered_map, I think.
#endif
#endif

#if USE_TR1_UNORDERED
#  include <tr1/unordered_set>
#  include <tr1/unordered_map>
#  define UNORDERED_NS_OPEN namespace std { namespace tr1 {
#  define UNORDERED_NS_CLOSE }}
#  define UNORDERED_NS ::std::tr1
#else
#  include <boost/unordered_map.hpp>
#  include <boost/unordered_set.hpp>
#  define UNORDERED_NS_OPEN namespace boost {
#  define UNORDERED_NS_CLOSE }
#  define UNORDERED_NS ::boost
#endif
#endif

namespace graehl {

// Containers:

/* todo: could extend like so:
   template <class A=std::allocator<void> >
   struct VectorS {
   template <class T> struct container {
   typedef dynamic_array<T,typename A::template rebind<T>::type> type;
   };
   };


*/

struct VectorS {
  template <class T> struct container {
    typedef std::vector<T> type;
  };
};

template <class T,class A>
void add(std::vector<T,A> &v,T const& t) {
  v.push_back(t);
}

template <class T,class A>
T & last_added(std::vector<T,A> &v) {
  return v.back();
}


inline VectorS vectorS() {
  return VectorS();
}

struct ListS {
  template <class T> struct container {
    typedef std::list<T> type;
  };
};

template <class T,class A>
void add(std::list<T,A> &v,T const& t) {
  v.push_back(t);
}

template <class T,class A>
T & last_added(std::list<T,A> &v) {
  return v.back();
}

inline ListS listS() {
  return ListS();
}

struct SlistS {
  template <class T> struct container {
    typedef GRAEHL_STD_SLIST<T> type;
  };
};

inline SlistS slistS() {
  return SlistS();
}

template <class T,class A>
void add(GRAEHL_STD_SLIST<T,A> &v,T const& t) {
  v.push_front(t);
}

template <class T,class A>
T & last_added(GRAEHL_STD_SLIST<T,A> &v) {
  return v.front();
}


struct SetS {
  template <class K> struct set {
    typedef std::set<K> type;
  };
};

inline SetS setS() {
  return SetS();
}

template <class T,class A,class B>
std::pair<typename std::set<T,A,B>::iterator,bool> add(std::set<T,A,B> &v,T const& t) {
  return v.insert(t);
}


struct UsetS {
  template <class K> struct set {
    typedef UNORDERED_NS::unordered_set<K,boost::hash<K> > type;
  };
};

inline UsetS usetS() {
  return UsetS();
}

template <class T,class A,class B,class C>
std::pair<typename UNORDERED_NS::unordered_set<T,A,B,C>::iterator,bool>
add(UNORDERED_NS::unordered_set<T,A,B,C> &v,T const& t) {
  return v.insert(t);
}

// Maps:


struct MapS {
  template <class K,class V> struct map {
    typedef std::map<K,V> type;
  };
};

inline MapS mapS() {
  return MapS();
}

struct UmapS {
  template <class K,class V> struct map {
    typedef UNORDERED_NS::unordered_map<K,V,boost::hash<K> > type;
  };
};

inline UmapS umapS() {
  return UmapS();
}

template <class M,class K>
typename M::mapped_type *find_second(const M& ht,const K& first)
{
  typedef typename M::mapped_type ret;
  typename M::const_iterator i=ht.find(first);
  if (i!=ht.end()) {
    return (ret *)&(i->second);
  } else
    return NULL;
}

// traits for map-like things that have different find() and insert() results than the usual:

template <class K>
struct map_traits;

template <class K,class V>
struct map_traits<std::map<K,V> > {
  typedef std::map<K,V> type;
  typedef typename type::iterator find_result_type;
  typedef std::pair<typename type::iterator,bool> insert_result_type;
};

template <class K,class V>
inline
typename map_traits<std::map<K,V> >::insert_result_type insert(std::map<K,V>& ht,const K& first,const V &v=V())
{
  return ht.insert(std::pair<K,V>(first,v));
}

template <class K,class V,class H,class E>
struct map_traits<UNORDERED_NS::unordered_map<K,V,H,E> > {
  typedef UNORDERED_NS::unordered_map<K,V,H,E> type;
  typedef typename type::iterator find_result_type;
  typedef std::pair<typename type::iterator,bool> insert_result_type;
};

template <class K,class V,class H,class E>
inline
typename map_traits<UNORDERED_NS::unordered_map<K,V,H,E> >::insert_result_type insert(UNORDERED_NS::unordered_map<K,V,H,E>& ht,const K& first,const V &v=V())
{
  return ht.insert(std::pair<K,V>(first,v));
}

template <class type,class find_result_type>
inline
bool found(const type &ht,find_result_type f) {
  return f==ht.end();
}

// not a template member because we don't want pointer casting (e.g. void *) to change hash val
template <class P>
struct ptr_hash {
  std::size_t operator()(void *p) const {
    return (P const*)p-(P const*)0;
  }
};

}//ns

#endif
