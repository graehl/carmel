#ifndef _PROPERTY_HPP
#define _PROPERTY_HPP

//#include <boost/property_map.hpp>


#ifdef TEST
#include "test.hpp"
#endif

template<class C>
struct property_factory {

};

/* usage: 
 ArrayPMapImp<V,O> p;
 graph_algo(g,boost::ref(p));
 */
template <class V,class O>
struct ArrayPMapImp : public 
  boost::put_get_helper<typename D &,ArrayPMapImp<V,O> > 
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
  D & operator [](key_type k) {
    return vals[get(ind,k)];
  }
private:
  ArrayPMapImp(Self &s) {}
};


/* usage:  
 Factory factory;
 typedef typename Factory::rebind<DataType> DFactory;
 typedef typename DFactory::implementation Imp;
 typedef typename DFactory::reference PMap;
 Imp imp(factory);
 PMap pmap(imp);
 ... use pmap
 */

template <class offset_map>
struct ArrayPMapFactory : public std::pair<unsigned,offset_map> {
  ArrayPMapFactory(unsigned s,offset_map o=offset_map()) : std::pair<unsigned,offset_map>(s,o) {}
  template <class R>
  struct rebind {    
    typedef ArrayPMapImp<R,O> implementation;
    typedef boost::reference_wrapper<other> reference;
  };
};

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

template <class G,class P1,class P2>
void copy_vertex_pmap(G &g,P1 a,P2 b) {
  visit<typename graph_traits<G>::vertex_descriptor>(IndexedCopier(a,b));
}

template <class G,class P1,class P2>
void copy_edge_pmap(G &g,P1 a,P2 b) {
  visit<typename graph_traits<G>::edge_descriptor>(IndexedCopier(a,b));
}

template <class G,class P1,class P2>
void copy_hyperarc_pmap(G &g,P1 a,P2 b) {
  visit<typename graph_traits<G>::hyperarc_descriptor>(IndexedCopier(a,b));
}


#ifdef TEST
BOOST_AUTO_UNIT_TEST( PROPERTY )
{
}
#endif

#endif
