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
#ifndef GRAEHL_SHARED__CONTAINERS_HPP
#define GRAEHL_SHARED__CONTAINERS_HPP
#pragma once

/* container selectors (all assume default allocator, but TODO-could change to template on allocator).

   usage:

   template <class V, class contS>
   typename contS::template container<V>::type c;

*/

#include <boost/config.hpp>
#include <limits.h>
#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

#include <deque>
#include <list>
#ifdef BOOST_HAS_SLIST
#include BOOST_SLIST_HEADER
#define GRAEHL_STD_SLIST BOOST_STD_EXTENSION_NAMESPACE::slist
#define GRAEHL_STD_SLIST_IS_LIST 0
#else
#define GRAEHL_STD_SLIST std::list
#define GRAEHL_STD_SLIST_IS_LIST 1
#endif

#include <boost/functional/hash.hpp>
#include <graehl/shared/unordered.hpp>
#include <map>
#include <set>
#include <vector>

namespace graehl {

// Containers:
struct VectorS {
  template <class T>
  struct container {
    typedef std::vector<T> type;
  };
};

template <class T, class A>
void add(std::vector<T, A>& v, T const& t) {
  v.push_back(t);
}

template <class T, class A>
T& last_added(std::vector<T, A>& v) {
  return v.back();
}


inline VectorS vectorS() {
  return VectorS();
}

struct ListS {
  template <class T>
  struct container {
    typedef std::list<T> type;
  };
};

template <class T, class A>
void add(std::list<T, A>& v, T const& t) {
  v.push_back(t);
}

template <class T, class A>
T& last_added(std::list<T, A>& v) {
  return v.back();
}

struct DequeS {
  template <class T>
  struct container {
    typedef std::deque<T> type;
  };
};

template <class T, class A>
void add(std::deque<T, A>& v, T const& t) {
  v.push_back(t);
}

template <class T, class A>
T& last_added(std::deque<T, A>& v) {
  return v.back();
}

inline DequeS dequeS() {
  return DequeS();
}

struct SlistS {
  template <class T>
  struct container {
    typedef GRAEHL_STD_SLIST<T> type;
  };
};

inline SlistS slistS() {
  return SlistS();
}

#if !GRAEHL_STD_SLIST_IS_LIST
template <class T, class A>
void add(GRAEHL_STD_SLIST<T, A>& v, T const& t) {
  v.push_front(t);
}

template <class T, class A>
T& last_added(GRAEHL_STD_SLIST<T, A>& v) {
  return v.front();
}
#endif

struct SetS {
  template <class K>
  struct set {
    typedef std::set<K> type;
  };
};

inline SetS setS() {
  return SetS();
}

template <class T, class A, class B>
std::pair<typename std::set<T, A, B>::iterator, bool> add(std::set<T, A, B>& v, T const& t) {
  return v.insert(t);
}


struct UsetS {
  template <class K>
  struct set {
    typedef GRAEHL_UNORDERED_NS::unordered_set<K, boost::hash<K>> type;
  };
};

inline UsetS usetS() {
  return UsetS();
}

template <class T, class A, class B, class C>
std::pair<typename GRAEHL_UNORDERED_NS::unordered_set<T, A, B, C>::iterator, bool>
add(GRAEHL_UNORDERED_NS::unordered_set<T, A, B, C>& v, T const& t) {
  return v.insert(t);
}

// Maps:


struct MapS {
  template <class K, class V>
  struct map {
    typedef std::map<K, V> type;
  };
};

inline MapS s() {
  return MapS();
}

struct UmapS {
  template <class K, class V>
  struct map {
    typedef GRAEHL_UNORDERED_NS::unordered_map<K, V, boost::hash<K>> type;
  };
};

inline UmapS umapS() {
  return UmapS();
}

template <class M, class K>
typename M::mapped_type* find_second(M const& ht, K const& first) {
  typedef typename M::mapped_type ret;
  typename M::const_iterator i = ht.find(first);
  if (i != ht.end()) {
    return (ret*)&(i->second);
  } else
    return NULL;
}

// traits for map-like things that have different find() and insert() results than the usual:

template <class Map>
struct map_traits;

template <class K, class V>
struct map_traits<std::map<K, V>> {
  typedef std::map<K, V> type;
  typedef typename type::iterator find_result_type;
  typedef std::pair<typename type::iterator, bool> insert_result_type;
};

template <class K, class V>
inline typename map_traits<std::map<K, V>>::insert_result_type insert(std::map<K, V>& ht, K const& first,
                                                                      V const& v = V()) {
  return ht.insert(std::pair<K, V>(first, v));
}

template <class K, class V, class H, class E>
struct map_traits<GRAEHL_UNORDERED_NS::unordered_map<K, V, H, E>> {
  typedef GRAEHL_UNORDERED_NS::unordered_map<K, V, H, E> type;
  typedef typename type::iterator find_result_type;
  typedef std::pair<typename type::iterator, bool> insert_result_type;
};

template <class K, class V, class H, class E>
inline typename map_traits<GRAEHL_UNORDERED_NS::unordered_map<K, V, H, E>>::insert_result_type
insert(GRAEHL_UNORDERED_NS::unordered_map<K, V, H, E>& ht, K const& first, V const& v = V()) {
  return ht.insert(std::pair<K, V>(first, v));
}

template <class type, class find_result_type>
inline bool found(type const& ht, find_result_type f) {
  return f == ht.end();
}


}

#endif
