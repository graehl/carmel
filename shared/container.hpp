#ifndef _CONTAINER_HPP
#define _CONTAINER_HPP

#include "byref.hpp"

template <class M,class F>
void enumerate(const M& m,F f) {
  for (typename M::const_iterator i=m.begin();i!=m.end();++i)
    deref(f)(*i);
}

template <class M,class F>
void enumerate(M& m,F f) {
  for (typename M::iterator i=m.begin();i!=m.end();++i)
    deref(f)(*i);
}


// Assoc. Maps:
#include <map>
#include "2hash.h"


struct HashS {
  template <class K,class V> struct map {
    typedef HashTable<K,V> type;
    typedef typename type::find_return_type find_return_type;
    typedef typename type::insert_return_type insert_return_type;
  };
};


struct MapS {
  template <class K,class V> struct map {
    typedef std::map<K,V> type;
    typedef typename type::iterator find_return_type;
    typedef std::pair<typename type::iterator,bool> insert_return_type;
  };
};

template <class K>
struct map_traits;

template <class K,class V>
struct map_traits<std::map<K,V> > {
  typedef std::map<K,V> type;
  typedef typename type::iterator find_return_type;
  typedef std::pair<typename type::iterator,bool> insert_return_type;
};

template <class type,class find_return_type>
inline
bool found(const type &ht,find_return_type f) {
  return f==ht.end();
}

template <class K,class V,class H,class P,class A>
inline
bool found(const HashTable<K,V,H,P,A> &ht,typename HashTable<K,V,H,P,A>::find_return_type f) {
  return f;
}

template <class K,class V,class H,class P,class A>
struct map_traits< HashTable<K,V,H,P,A> > {
  typedef HashTable<K,V,H,P,A> type;
  typedef typename type::find_return_type find_return_type;
  typedef typename type::insert_return_type insert_return_type;
};


template <class V,class K>
inline typename std::map<K,V>::mapped_type *find_second(const std::map<K,V>& ht,const K& first)
{
  typedef std::map<K,V> M;
  typedef typename M::mapped_type ret;
  typename M::const_iterator i=ht.find(first);
  if (i!=ht.end()) {
    return (ret *)&(i->second);
  } else
    return NULL;
}


template <class K,class V>
inline
typename map_traits<std::map<K,V> >::insert_return_type insert(std::map<K,V>& ht,const K& first,const V &v=V())
{
  return ht.insert(std::pair<K,V>(first,v));
}


// Containers:

#include "dynarray.h"

struct VectorS {
  template <class T> struct container {
        typedef DynamicArray<T> type;
  };
};

#include "list.h"

struct ListS {
  template <class T> struct container {
        typedef List<T> type;
  };
};


#ifdef TEST
#include "test.hpp"
#endif

#ifdef TEST
template <class S>
void maptest() {
  typedef typename S::template map<int,int>::type map;
  map m;
  BOOST_CHECK(  insert(m,1,0).second);
  BOOST_CHECK(find_second(m,1)!=NULL);
  BOOST_CHECK(*(find_second(m,1))==0);
  BOOST_CHECK(  !insert(m,1,2).second);
  BOOST_CHECK(  insert(m,1,2).first != m.end());// == m.find(m,1)
  BOOST_CHECK( insert(m,3,4).second);
  BOOST_CHECK(m.find(1)->first == 1);
  BOOST_CHECK(m.find(3)->first == 3);
  BOOST_CHECK(m.find(3)->second == 4);
  BOOST_CHECK(find_second(m,3) == &m.find(3)->second);
  BOOST_CHECK(m.find(0)==m.end());
  test_counter n;
  enumerate(m,n);
  BOOST_CHECK(n.n==2);

}

template <class S>
void containertest() {
  typedef typename S::template container<unsigned>::type cont;
  cont c;
  BOOST_CHECK(c.size()==0);
  c.push(10);
  c.push(9);
  BOOST_CHECK(c.size()==2);
  BOOST_CHECK(c.top()==9);
  bool nine=false,ten=false;
  for(typename cont::iterator i=c.begin(),e=c.end();i!=e;++i) {
    if (*i==9)
      nine=true;
    else if (*i==10)
      ten=true;
    else
      BOOST_CHECK(false);
  }
  test_counter n;
  enumerate(c,n);
  BOOST_CHECK(n.n==2);

  BOOST_CHECK(nine&&ten);
}

BOOST_AUTO_UNIT_TEST( TEST_CONTAINER )
{
  maptest<HashS>();
  maptest<MapS>();
  containertest<ListS>();
  containertest<VectorS>();
}
#endif

#endif
