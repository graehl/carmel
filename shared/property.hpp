#ifndef _PROPERTY_HPP
#define _PROPERTY_HPP

#include <boost/property_map.hpp>
#include <boost/ref.hpp>
#include "dynarray.h"

#ifdef TEST
#include "test.hpp"
#endif


template <class K>
struct OffsetMap {
  K begin; // have to compute index anyhow because size of K might differ from size of V
  //FixedArray<V> values;
  unsigned index(K p) const {
    Assert(p>=begin);
    //Assert(p<begin+values.size());
    return p-begin;
  }
  OffsetMap(K beg) : begin(beg) {
  }
  typedef boost::readable_property_map_tag category;
  typedef unsigned value_type;
  typedef K key_type;
  unsigned operator [](K p) const {
    return index(p);
  }
};

template <class K>
unsigned get(OffsetMap<K> k,K p) {
  return k[p];
}


/* usage: 
 ArrayPMapImp<V,O> p;
 graph_algo(g,boost::ref(p));
 */
template <class V,class O=boost::identity_property_map>
struct ArrayPMapImp : public 
  boost::put_get_helper<V &,ArrayPMapImp<V,O> > 
{
  typedef ArrayPMapImp<V,O> Self;
  //typedef typename graph_traits<G>::hyperarc_descriptor key_type;
//  typedef ArrayPMap<V,O> property_map;
  typedef O offset_map;
  typedef typename O::key_type key_type; 
  typedef boost::lvalue_property_map_tag category;
  typedef V value_type;
  typedef V& reference;
  typedef std::pair<unsigned,offset_map> init_type;  
  offset_map ind;
  FixedArray<value_type> vals; // not copyable!
  //ArrayPMapImp(G &g) : ind(g), vals(num_hyperarcs(g)) { }
  
  ArrayPMapImp(unsigned size,offset_map o) : ind(o), vals(size) {}
  ArrayPMapImp(const init_type &init) : ind(init.second), vals(init.first) {}
  V & operator [](key_type k) {
    return vals[get(ind,k)];
  }
private:
  ArrayPMapImp(Self &s) {}
};

/*
template <class V,class O=boost::identity_property_map>
struct ArrayPMap {
  typedef ArrayPMapImp<V,O> Imp;
  typedef boost::reference_wrapper<Imp> type;
};
*/

/*
template <class V,class O>
struct ArrayPMap {
  typedef boost::reference_type<ArrayPMapImp<V,O> > type;
};
*/

/* usage:  
 Factory factory;
 typedef typename Factory::rebind<DataType> DFactory;
 typedef typename DFactory::implementation Imp;
 typedef typename DFactory::reference PMap;
 Imp imp(max_size,factory);
 PMap pmap(imp);
 ... use pmap
 */

template <class offset_map=boost::identity_property_map>
struct ArrayPMapFactory : public std::pair<unsigned,offset_map> {
  ArrayPMapFactory(unsigned s,offset_map o=offset_map()) : std::pair<unsigned,offset_map>(s,o) {}
  ArrayPMapFactory(const ArrayPMapFactory &o) : std::pair<unsigned,offset_map>(o) {}
  template <class R>
  struct rebind {    
    typedef ArrayPMapImp<R,offset_map> implementation;
    typedef boost::reference_wrapper<implementation> reference;
    // reference(implementation &i) constructor exists
    /*static reference pmap(implementation &i) {
      return reference(i);
    }*/
  };
};

template <class G,class ptag>
struct property_factory;

/*
template<class V,class O>
struct property_traits<boost::reference_wrapper<ArrayPMapImp<V,O> > {
  typedef ArrayPMapImp<V,O> Imp;
  typedef typename Imp::category category;
  typedef typename Imp::key_type key_type;
  typedef typename Imp::value_type value_type;

};
*/

template<class Imp>
struct property_traits<boost::reference_wrapper<Imp> > {
  typedef typename Imp::category category;
  typedef typename Imp::key_type key_type;
  typedef typename Imp::value_type value_type;
};

/*
template <class Imp>
struct RefPMap : public boost::reference_wrapper<Imp>{
  typedef typename Imp::category category;
  typedef typename Imp::key_type key_type;
  typedef typename Imp::value_type value_type;
  explicit RefPMap(Imp &a) : boost::reference_wrapper<Imp>(a) {}
};
*/



template <class P1,class P2>
struct IndexedCopier : public std::pair<P1,P2> {
  IndexedCopier(P1 a_,P2 b_) : std::pair(a_,b_) {}
  template<class I>
    void operator()(I i) {
      first[i] = second[i];
    }
};

template <class P1,class P2>
IndexedCopier<P1,P2> make_indexed_copier(P1 a,P2 b) {
  return IndexedCopier<P1,P2>(a,b);
}

template <class P1,class P2,class P3>
struct IndexedPairCopier {
  P1 a;P2 b;P3 c;  
  IndexedPairCopier(P1 a_,const P2 b_,const P3 c_) : a(a_),b(b_),c(c_) {}
  template<class I>
    void operator()(I i) {
      a[i].first = b[i];
      a[i].second = c[i];
    }
};

template <class P1,class P2,class P3>
IndexedPairCopier<P1,P2,P3> make_indexed_pair_copier(const P1 a,const P2 b,const P3 c) {
  return IndexedCopier<P1,P2,P3>(a,b,c);
}

#ifdef TEST
BOOST_AUTO_UNIT_TEST( PROPERTY )
{
}
#endif

#endif
