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

    enumerate for generic containers (and nested_enumerate for nested
    containers).  also, conatiner selectors (HashS and MapS for lookup tables,
    VectorS and ListS for sequences) usable as template arguments.
*/


#ifndef GRAEHL_SHARED__CONTAINER_HPP
#define GRAEHL_SHARED__CONTAINER_HPP
#pragma once

#include <graehl/shared/append.hpp>
#include <graehl/shared/containers.hpp>
#include <deque>
#include <list>
#include <map>
#include <vector>

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#endif

namespace graehl {

template <class C, class V>
inline void add(C& c, V const& v) {
  c.insert(v);
}

template <class V, class A, class Val>
inline void add(std::vector<V, A>& c, Val const& v) {
  c.push_back(v);
}

template <class V, class A, class Val>
inline void add(std::deque<V, A>& c, Val const& v) {
  c.push_back(v);
}

template <class V, class A, class Val>
inline void add(std::list<V, A>& c, Val const& v) {
  c.push_back(v);
}

template <class Key, class Findable>
bool contains(Findable const& set, Key const& key) {
  return set.find(key) != set.end();
}

template <class R, class C, class F>
R map(C const& c, F f) {
  R ret;
  for (auto& x : c)
    ret.push_back(f(x));
  return ret;
}

template <class Tag, class M, class F>
void nested_enumerate(M& m, F& f, Tag t) {
  for (typename M::iterator i = m.begin(); i != m.end(); ++i)
    for (typename M::value_type::iterator j = i->begin(); j != i->end(); ++j)
      f.visit(*j, t);
}

template <class Tag, class M, class F>
void enumerate(M& m, F& f, Tag t) {
  for (typename M::iterator i = m.begin(); i != m.end(); ++i)
    f.visit(*i, t);
}

template <class V, class K>
inline typename std::map<K, V>::mapped_type* find_second(const std::map<K, V>& ht, K const& first) {
  typedef std::map<K, V> M;
  typedef typename M::mapped_type ret;
  typename M::const_iterator i = ht.find(first);
  if (i != ht.end()) {
    return (ret*)&(i->second);
  } else
    return NULL;
}

template <class T>
inline bool container_equal(T const& v1, T const& v2, typename T::const_iterator* /*SFINAE*/ = 0) {
  if (v1.size() != v2.size())
    return false;
  for (typename T::const_iterator i1 = v1.begin(), i2 = v2.begin(), e1 = v1.end(); i1 != e1; ++i1, ++i2)
    if (!(*i1 == *i2))
      return false;
  return true;
}


// Containers:


#ifdef GRAEHL_TEST
#include <graehl/shared/small_vector.hpp>

template <class S>
void maptest() {
  typedef typename S::template map<int, int>::type map;
  map m;
  BOOST_CHECK(insert(m, 1, 0).second);
  BOOST_CHECK(find_second(m, 1) != NULL);
  BOOST_CHECK(*(find_second(m, 1)) == 0);
  BOOST_CHECK(!insert(m, 1, 2).second);
  BOOST_CHECK(insert(m, 1, 2).first != m.end()); // == m.find(m,1)
  BOOST_CHECK(insert(m, 3, 4).second);
  BOOST_CHECK(m.find(1)->first == 1);
  BOOST_CHECK(m.find(3)->first == 3);
  BOOST_CHECK(m.find(3)->second == 4);
  BOOST_CHECK(find_second(m, 3) == &m.find(3)->second);
  BOOST_CHECK(m.find(0) == m.end());
}

template <class S>
void containertest() {
  typedef typename S::template container<unsigned>::type cont;
  cont c;
  BOOST_CHECK(c.empty());
  c.push_back(10);
  c.push_back(9);
  BOOST_CHECK(c.size() == 2);
  {
    bool nine = false, ten = false;
    for (typename cont::const_iterator i = c.begin(), e = c.end(); i != e; ++i) {
      if (*i == 9)
        nine = true;
      else if (*i == 10)
        ten = true;
      else
        BOOST_CHECK(false);
    }

    BOOST_CHECK(nine && ten);
  }
  {
    bool nine = false, ten = false;
    for (typename cont::iterator i = c.begin(), e = c.end(); i != e; ++i) {
      if (*i == 9)
        nine = true;
      else if (*i == 10)
        ten = true;
      else
        BOOST_CHECK(false);
    }

    BOOST_CHECK(nine && ten);
  }
}

int plus1(int x) {
  return x - 10;
}
struct F {
  typedef unsigned result_type;
  unsigned operator()(int x) const { return x - 10; }
};


BOOST_AUTO_TEST_CASE(TEST_CONTAINER) {
  maptest<MapS>();
  containertest<ListS>();
  containertest<VectorS>();
  small_vector<int> a(10, 2);
  a[1] = 1;
  std::cout << a << "\n" << map<small_vector<int>>(a, plus1) << "\n";
}
#endif


} // namespace graehl

#endif
