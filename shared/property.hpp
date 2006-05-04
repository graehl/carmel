// some Boost Graph Library property maps
#ifndef PROPERTY_HPP
#define PROPERTY_HPP

#include <boost/property_map.hpp>
#include <graehl/shared/byref.hpp>
#include <graehl/shared/dynarray.h>
#include <utility>
#include <graehl/shared/genio.h>

#ifdef TEST
#include <graehl/shared/test.hpp>
#endif

namespace graehl {

template <class K>
struct OffsetMap {
  K begin; // have to compute index anyhow because size of K might differ from size of V
  unsigned index(K p) const {
    Assert(p>=begin);
    //Assert(p<begin+values.size());
    return (unsigned)(p-begin);
  }
  explicit OffsetMap(K beg) : begin(beg) {
  }
  /* // default constructor
  OffsetMap(const OffsetMap<K> &o) : begin(o.begin) {
    //DBPC("OffSetMap copy",begin);
  }
  */
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



template <class P>
typename P::value_type get(reference_wrapper<P> p,typename P::key_type k) {
  return get(deref(p),k);
}

template <class P>
void put(reference_wrapper<P> p,typename P::key_type k,typename P::value_type v) {
  return put(deref(p),k,v);
}



/* usage:
 ArrayPMapImp<V,O> p;
 graph_algo(g,boost::ref(p));
 */
template <class V,class O=boost::identity_property_map>
struct ArrayPMapImp
//: public  boost::put_get_helper<V &,ArrayPMapImp<V,O> >
{
  typedef ArrayPMapImp<V,O> Self;
  typedef reference_wrapper<Self> PropertyMap;
  //typedef typename graph_traits<G>::hyperarc_descriptor key_type;
//  typedef ArrayPMap<V,O> property_map;
  typedef O offset_map;
  typedef typename O::key_type key_type;
  typedef boost::lvalue_property_map_tag category;
  typedef V value_type;
  typedef V& reference;
  typedef std::pair<unsigned,offset_map> init_type;
  offset_map ind; // should also be copyable
  typedef fixed_array<value_type> Vals;
  Vals  vals; // copyable!
  //ArrayPMapImp(G &g) : ind(g), vals(num_hyperarcs(g)) { }

  ArrayPMapImp(unsigned size,offset_map o) : ind(o), vals(size) {}
  explicit ArrayPMapImp(const init_type &init) : ind(init.second), vals(init.first) {}
  operator Vals & ()  {
    return vals;
  }
  V & operator [](key_type k) const {
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4267 )
#endif
    return vals[get(ind,k)];
#ifdef _MSC_VER
#pragma warning( pop )
#endif
  }
  GENIO_print
  {
    o << vals;
    return GENIOGOOD;
  }
  GENIO_print_writer
  {
    return vals.print(o,w);
  }

  /*
  // default constructor
  ArrayPMapImp(const Self &s) : vals(s.vals), ind(s.ind) {
    //    DBPC("ArrayPMapImp copy",vals);
  }
  */
private:
  //  ArrayPMapImp(Self &s) : vals(s.vals) {}
};




template <class V,class O>
V get(const ArrayPMapImp<V,O> &p,typename ArrayPMapImp<V,O>::key_type k) {
  return p[k];
}

template <class V,class O>
void put(ArrayPMapImp<V,O> &p,typename ArrayPMapImp<V,O>::key_type k,V v) {
  p[k]=v;
}


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

namespace boost {
template<class Imp>
struct property_traits<boost::reference_wrapper<Imp> > {
  typedef typename Imp::category category;
  typedef typename Imp::key_type key_type;
  typedef typename Imp::value_type value_type;
};
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
  IndexedCopier(P1 a_,P2 b_) : std::pair<P1,P2>(a_,b_) {}
  template<class I>
    void operator()(I i) {
      (this->first)[i] = (this->second)[i];
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
      deref(a)[i].first = deref(b)[i];
      deref(a)[i].second = deref(c)[i];
    }
};

template <class P1,class P2,class P3>
IndexedPairCopier<P1,P2,P3> make_indexed_pair_copier(const P1 a,const P2 b,const P3 c) {
  return IndexedPairCopier<P1,P2,P3>(a,b,c);
}

#ifdef TEST
BOOST_AUTO_UNIT_TEST( PROPERTY )
{
}
#endif
}

#endif
