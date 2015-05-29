// Copyright 2014 Jonathan Graehl - http://graehl.org/
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
#ifndef GRAEHL_SHARED__CONTAINERS_HPP
#define GRAEHL_SHARED__CONTAINERS_HPP
#pragma once

/* container selectors (all assume default allocator, but TODO - could change to template on allocator).

   usage:

   template <class V, class contS>
   typename contS::template container<V>::type c;

*/

#include <boost/config.hpp>

#include <limits.h>
#ifndef CHAR_BIT
# define CHAR_BIT 8
#endif

#include <list>
#include <deque>
#ifdef BOOST_HAS_SLIST
#include BOOST_SLIST_HEADER
# define GRAEHL_STD_SLIST BOOST_STD_EXTENSION_NAMESPACE::slist
# define GRAEHL_STD_SLIST_IS_LIST 0
#else
# define GRAEHL_STD_SLIST std::list
# define GRAEHL_STD_SLIST_IS_LIST 1
#endif

#include <vector>
#include <map>
#include <set>

#include <boost/functional/hash.hpp>

#ifndef UNORDERED_NS
#ifndef USE_TR1_UNORDERED
#if defined(_WIN32) || __clang__
// mac clang lacks tr1::unordered
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
   typedef dynamic_array<T, typename A::template rebind<T>::type> type;
   };
   };


*/

struct VectorS {
  template <class T> struct container {
    typedef std::vector<T> type;
  };
};

template <class T, class A>
void add(std::vector<T, A> &v, T const& t) {
  v.push_back(t);
}

template <class T, class A>
T & last_added(std::vector<T, A> &v) {
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

template <class T, class A>
void add(std::list<T, A> &v, T const& t) {
  v.push_back(t);
}

template <class T, class A>
T & last_added(std::list<T, A> &v) {
  return v.back();
}

struct DequeS {
  template <class T> struct container {
    typedef std::deque<T> type;
  };
};

template <class T, class A>
void add(std::deque<T, A> &v, T const& t) {
  v.push_back(t);
}

template <class T, class A>
T & last_added(std::deque<T, A> &v) {
  return v.back();
}

inline DequeS dequeS() {
  return DequeS();
}

struct SlistS {
  template <class T> struct container {
    typedef GRAEHL_STD_SLIST<T> type;
  };
};

inline SlistS slistS() {
  return SlistS();
}

#if !GRAEHL_STD_SLIST_IS_LIST
template <class T, class A>
void add(GRAEHL_STD_SLIST<T, A> &v, T const& t) {
  v.push_front(t);
}

template <class T, class A>
T & last_added(GRAEHL_STD_SLIST<T, A> &v) {
  return v.front();
}
#endif

struct SetS {
  template <class K> struct set {
    typedef std::set<K> type;
  };
};

inline SetS setS() {
  return SetS();
}

template <class T, class A, class B>
std::pair<typename std::set<T, A, B>::iterator, bool> add(std::set<T, A, B> &v, T const& t) {
  return v.insert(t);
}


struct UsetS {
  template <class K> struct set {
    typedef UNORDERED_NS::unordered_set<K, boost::hash<K> > type;
  };
};

inline UsetS usetS() {
  return UsetS();
}

template <class T, class A, class B, class C>
std::pair<typename UNORDERED_NS::unordered_set<T, A, B, C>::iterator, bool>
add(UNORDERED_NS::unordered_set<T, A, B, C> &v, T const& t) {
  return v.insert(t);
}

// Maps:


struct MapS {
  template <class K, class V> struct map {
    typedef std::map<K, V> type;
  };
};

inline MapS s() {
  return MapS();
}

struct UmapS {
  template <class K, class V> struct map {
    typedef UNORDERED_NS::unordered_map<K, V, boost::hash<K> > type;
  };
};

inline UmapS umapS() {
  return UmapS();
}

template <class M, class K>
typename M::mapped_type *find_second(M const& ht, K const& first)
{
  typedef typename M::mapped_type ret;
  typename M::const_iterator i=ht.find(first);
  if (i!=ht.end()) {
    return (ret *)&(i->second);
  } else
    return NULL;
}

// traits for map-like things that have different find() and insert() results than the usual:

template <class Map>
struct map_traits;

template <class K, class V>
struct map_traits<std::map<K, V> > {
  typedef std::map<K, V> type;
  typedef typename type::iterator find_result_type;
  typedef std::pair<typename type::iterator, bool> insert_result_type;
};

template <class K, class V>
inline
typename map_traits<std::map<K, V> >::insert_result_type insert(std::map<K, V>& ht, K const& first, V const& v=V())
{
  return ht.insert(std::pair<K, V>(first, v));
}

template <class K, class V, class H, class E>
struct map_traits<UNORDERED_NS::unordered_map<K, V, H, E> > {
  typedef UNORDERED_NS::unordered_map<K, V, H, E> type;
  typedef typename type::iterator find_result_type;
  typedef std::pair<typename type::iterator, bool> insert_result_type;
};

template <class K, class V, class H, class E>
inline
typename map_traits<UNORDERED_NS::unordered_map<K, V, H, E> >::insert_result_type insert(UNORDERED_NS::unordered_map<K, V, H, E>& ht, K const& first, V const& v=V())
{
  return ht.insert(std::pair<K, V>(first, v));
}

template <class type, class find_result_type>
inline
bool found(type const& ht, find_result_type f) {
  return f==ht.end();
}

// not a template member because we don't want pointer casting (e.g. void *) to change hash val
template <class P>
struct ptr_hash {
  std::size_t operator()(void *p) const {
    return (P const*)p-(P const*)0;
  }
};

static const boost::uint32_t golden_ratio_fraction_32u=2654435769U; // (floor of 2^32/golden_ratio)
inline void uint32_hash_inplace(boost::uint32_t &a)
{
    a *= golden_ratio_fraction_32u; // mixes the lower bits into the upper bits, reversible
    a ^= (a >> 16); // gets some of the goodness back into the lower bits, reversible
}

inline boost::uint32_t uint32_hash_fast(boost::uint32_t a)
{
  uint32_hash_inplace(a);
  return a;
}

inline std::size_t hash_rotate_left(std::size_t x)  // there is probably an ASM instruction for this; does -O3 find it?
{
  return x << 1 | x>>((sizeof(std::size_t))*CHAR_BIT-1);
}

inline void mix_hash_inplace(std::size_t &a, std::size_t b)
{
    a ^= hash_rotate_left(b);
}

inline std::size_t mix_hash_fast(std::size_t a, std::size_t b)
{
    a ^= hash_rotate_left(b);
    return a;
}


}

#endif
