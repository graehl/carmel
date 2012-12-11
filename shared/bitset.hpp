#ifndef GRAEHL_SHARED__BITSET_HPP
#define GRAEHL_SHARED__BITSET_HPP

/* use dynamic_bitset as boost::range of set bit indices (visit the indices that were set) - not the same as an iter returning 1 or 0 everywhere*/

#include <boost/dynamic_bitset.hpp>
#include <iterator>
#include <boost/shared_ptr.hpp>

namespace graehl {

typedef boost::dynamic_bitset<std::size_t> bitset;


template <class Bitset>
struct bitset_pmap {
  typedef boost::shared_ptr<Bitset> B;
  B p;
  explicit bitset_pmap(std::size_t n) : p(new Bitset(n)) {}
  explicit bitset_pmap(B p) : p(p) {}
  typedef std::size_t key_type;
  typedef bool value_type;
  typedef value_type const& reference;
  typedef boost::read_write_property_map_tag category;
  friend inline value_type get(bitset_pmap const& b,key_type i) {
    return b.test(i);
  }
  friend inline value_type put(bitset_pmap const& b,key_type i,value_type v) {
    b.set(i,v);
  }
};

template <class I,class A=std::allocator<I> >
struct set_bits_iter : std::iterator<std::forward_iterator_tag,bool> {
  typedef boost::dynamic_bitset<I,A> B;
  B const* b;
  typedef std::size_t S;
  S i;
//  set_bits_iter(set_bits_iter const& o) : b(o.b),i(o.i) {}
  set_bits_iter() : b(),i(B::npos) {}
  explicit set_bits_iter(B const& b) : b(&b),i(b.find_first()) {}
  typedef set_bits_iter Self;
  inline void advance() {
    i=b->find_next(i);
  }
  Self & operator ++() {
    advance();
  }
  Self operator ++(int) {
    set_bits_iter r=*this;
    advance();
    return r;
  }
  S operator *() const {
    return i;
  }
  bool operator == (set_bits_iter const& o) const {
    assert(o.b==0 || b == 0 || o.b == b);
    return i==o.i;
  }
  bool operator != (set_bits_iter const& o) const {
    assert(o.b==0 || b == 0 || o.b == b);
    return i!=o.i;
  }
};

template <class I,class A>
graehl::set_bits_iter<I,A> begin(boost::dynamic_bitset<I,A> const& c) {
  return graehl::set_bits_iter<I,A>(c);
}

template <class I,class A>
graehl::set_bits_iter<I,A> end(boost::dynamic_bitset<I,A> const& c) {
  return graehl::set_bits_iter<I,A>();
}

}//ns

namespace boost {
//ADL for bitset means we need to be in boost ns

template <class I,class A>
graehl::set_bits_iter<I,A> range_begin(boost::dynamic_bitset<I,A> const& c) {
  return graehl::set_bits_iter<I,A>(c);
}

template <class I,class A>
graehl::set_bits_iter<I,A> range_end(boost::dynamic_bitset<I,A> const& c) {
  return graehl::set_bits_iter<I,A>();
}

// mutable dup
template <class I,class A>
graehl::set_bits_iter<I,A> range_begin(boost::dynamic_bitset<I,A> & c) {
  return graehl::set_bits_iter<I,A>(c);
}

template <class I,class A>
graehl::set_bits_iter<I,A> range_end(boost::dynamic_bitset<I,A> & c) {
  return graehl::set_bits_iter<I,A>();
}

//end dup

template <class I,class A>
std::size_t range_calculate_size(boost::dynamic_bitset<I,A> const& c) {
  return c.count();
}

template <class I,class A>
struct range_const_iterator<boost::dynamic_bitset<I,A> > {
  typedef graehl::set_bits_iter<I,A> type;
};

template <class I,class A>
struct range_mutable_iterator<boost::dynamic_bitset<I,A> > {
  typedef graehl::set_bits_iter<I,A> type;
};

}


#endif
