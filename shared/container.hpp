// visitor-style enumerate for generic containers (and nested_enumerate for nested containers).  also, conatiner selectors (HashS and MapS for lookup tables, VectorS and ListS for sequences) usable as template arguments
#ifndef CONTAINER_HPP
#define CONTAINER_HPP

#include <graehl/shared/byref.hpp>
#include <map>
#include <graehl/shared/2hash.h>
#include <graehl/shared/list.h>
#include <graehl/shared/dynarray.h>

#ifdef TEST
#include <graehl/shared/test.hpp>
#endif

namespace graehl {

template <template<class X,class=std::allocator<X> >class R,class C,class F>
R<typename F::return_type,typename C::allocator_type::template rebind<typename F::return_type>::type> map(C const& c,F f)
{
    //
    R<typename F::return_type,typename C::allocator_type::template rebind<typename F::return_type>::type> ret;
    for (typename C::const_iterator i=c.begin(),e=c.end();i!=e;++i)
        ret.push_back(f(*i));
    return ret;
}

template <class R,class C,class F>
R map(C const& c,F f)
{
    R ret;
    for (typename C::const_iterator i=c.begin(),e=c.end();i!=e;++i)
        ret.push_back(f(*i));
    return ret;
}

template <class Tag,class M,class F>
void nested_enumerate(M& m,F &f,Tag t) {
    for (typename M::iterator i=m.begin();i!=m.end();++i)
        for (typename M::value_type::iterator j=i->begin();j!=i->end();++j)
            f.visit(*j,t);
}

template <class Tag,class M,class F>
void enumerate(M& m,F &f,Tag t) {
    for (typename M::iterator i=m.begin();i!=m.end();++i)
        f.visit(*i,t);
}


// for containers of containers where you want to visit every element
template <class M,class F>
void nested_enumerate(const M& m,F f) {
    typedef typename M::value_type Inner;
    for (typename M::const_iterator i=m.begin();i!=m.end();++i)
        for (typename Inner::const_iterator j=i->begin();j!=i->end();++j)
            deref(f)(*j);
}

template <class M,class F>
void nested_enumerate(M& m,F f) {
    typedef typename M::value_type Inner;
    for (typename M::iterator i=m.begin();i!=m.end();++i)
        for (typename Inner::iterator j=i->begin();j!=i->end();++j)
            deref(f)(*j);
}


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

template <class T>
inline bool container_equal(const T &v1, const T &v2,typename T::const_iterator *SFINAE=0)
{
    if (v1.size()!=v2.size())
        return false;
    for (typename T::const_iterator i1=v1.begin(),i2=v2.begin(),e1=v1.end();
         i1!=e1;++i1,++i2)
        if (!(*i1 == *i2))
            return false;
    return true;
}


// Containers:


struct VectorS {
  template <class T> struct container {
        typedef dynamic_array<T> type;
  };
};


struct ListS {
  template <class T> struct container {
        typedef List<T> type;
  };
};



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
  {

  bool nine=false,ten=false;
  for(typename cont::const_iterator i=c.begin(),e=c.end();i!=e;++i) {
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
  {

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


}

int plus1(int x) { return x-10; }
struct F
{
    typedef unsigned return_type;
    unsigned operator()(int x) const
    {
        return x-10;
    }
};


BOOST_AUTO_TEST_CASE( TEST_CONTAINER )
{
  maptest<HashS>();
  maptest<MapS>();
  containertest<ListS>();
  containertest<VectorS>();
  dynamic_array<int> a(10,2);
  a[1]=1;
  std::cout << a << "\n" << map<dynamic_array<int> >(a,plus1) << "\n";
  std::cout << a << "\n" << map<dynamic_array>(a,F()) << "\n";
}
#endif
}

#endif
